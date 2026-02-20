/**
 * Testiranje PCA9685 I2C Servo Drivera
 * Namjena: Simultano i polagano pokretanje 4 servo motora na 90 stupnjeva.
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Kreiranje objekta za driver na standardnoj I2C adresi 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// Definiranje radnih ciklusa u mikrosekundama
// Većina servo motora koristi 500us za 0° i 2500us za maksimalni kut
#define MIN_PULS 500  
#define MAX_PULS 2500 

// Mapiranje kanala na driveru
const int CH_BAZA = 0;
const int CH_RAME = 1;
const int CH_LAKAT = 2;
const int CH_ZGLOB = 3;
const int CH_HVATALJKA = 4; // Spremno za budućnost

// Definiranje pozicija (Ispravljena sintaksa niza u C++)
// Ovo je neophodno za točno pamćenje kuteva.
int pozicijaParking[5] = {135, 145, 25, 90, 90};
int pozicija1[5]       = {60,  60,  90, 45, 90};  // Prva pozicija (bivša 'Nova')
int pozicija2[5] = {230, 60, 120, 30, 90};
int pozicija3[5] = {220, 120, 50, 150, 90};  // Treća pozicija (primjer)

/**
 * Pomoćna funkcija koja preračunava željene stupnjeve u mikrosekunde
 * @param stupnjevi Željeni kut
 * @param maxStupnjeva Fizički maksimum motora (180 ili 270)
 */
int pretvoriStupnjeveUPulseve(int stupnjevi, int maxStupnjeva) {
  // Ograničavanje unosa za sigurnost
  if (stupnjevi > maxStupnjeva) stupnjevi = maxStupnjeva;
  if (stupnjevi < 0) stupnjevi = 0;
  
  return map(stupnjevi, 0, maxStupnjeva, MIN_PULS, MAX_PULS);
}

/**
 * Funkcija za lagano pomicanje robota iz jedne pozicije u drugu
 */
void premjestiRobot(int* pocetnaPoz, int* ciljnaPoz, int brojKoraka = 150, int pauzaPoKorakuMs = 20) {
  int startBaza = pretvoriStupnjeveUPulseve(pocetnaPoz[0], 270);
  int startRame = pretvoriStupnjeveUPulseve(pocetnaPoz[1], 180);
  int startLakat = pretvoriStupnjeveUPulseve(pocetnaPoz[2], 180);
  int startZglob = pretvoriStupnjeveUPulseve(pocetnaPoz[3], 180);

  int ciljBaza = pretvoriStupnjeveUPulseve(ciljnaPoz[0], 270);
  int ciljRame = pretvoriStupnjeveUPulseve(ciljnaPoz[1], 180);
  int ciljLakat = pretvoriStupnjeveUPulseve(ciljnaPoz[2], 180);
  int ciljZglob = pretvoriStupnjeveUPulseve(ciljnaPoz[3], 180);

  for (int i = 0; i <= brojKoraka; i++) {
    int trenutnoBaza = startBaza + ((ciljBaza - startBaza) * (float)i / brojKoraka);
    int trenutnoRame = startRame + ((ciljRame - startRame) * (float)i / brojKoraka);
    int trenutnoLakat = startLakat + ((ciljLakat - startLakat) * (float)i / brojKoraka);
    int trenutnoZglob = startZglob + ((ciljZglob - startZglob) * (float)i / brojKoraka);

    pwm.writeMicroseconds(CH_BAZA, trenutnoBaza);
    pwm.writeMicroseconds(CH_RAME, trenutnoRame);
    pwm.writeMicroseconds(CH_LAKAT, trenutnoLakat);
    pwm.writeMicroseconds(CH_ZGLOB, trenutnoZglob);

    delay(pauzaPoKorakuMs);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("--- Inicijalizacija PCA9685 Servo Drivera ---");

  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50); // Servo motori rade na 50 Hz (20ms period)
  delay(10);

  // Zbog sigurnosti, prvo sve motore postavljamo u PARKING poziciju
  Serial.println("Postavljanje u PARKING poziciju...");
  pwm.writeMicroseconds(CH_BAZA, pretvoriStupnjeveUPulseve(pozicijaParking[0], 270));
  pwm.writeMicroseconds(CH_RAME, pretvoriStupnjeveUPulseve(pozicijaParking[1], 180));
  pwm.writeMicroseconds(CH_LAKAT, pretvoriStupnjeveUPulseve(pozicijaParking[2], 180));
  pwm.writeMicroseconds(CH_ZGLOB, pretvoriStupnjeveUPulseve(pozicijaParking[3], 180));
  
  // Dajemo im 2 sekunde da dođu na početnu poziciju prije početka testa
  delay(2000); 
}

void loop() {
  Serial.println("Krećem s PARKING na POZICIJU 1...");
  premjestiRobot(pozicijaParking, pozicija1);
  delay(2000);

  Serial.println("Krećem s POZICIJE 1 na POZICIJU 2...");
  premjestiRobot(pozicija1, pozicija2);
  delay(2000);

  Serial.println("Krećem s POZICIJE 2 na POZICIJU 3...");
  premjestiRobot(pozicija2, pozicija3);
  delay(2000);

  Serial.println("Povratak s POZICIJE 3 na PARKING...");
  premjestiRobot(pozicija3, pozicijaParking);
  
  Serial.println("Ciklus zavrsen. Cekam 3 sekunde prije ponavljanja...");
  delay(3000);
}