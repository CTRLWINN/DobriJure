#include "Kretanje.h"
#include "Motori.h"
#include "Enkoderi.h"
#include "IMU.h"

// Konfiguracija
// Konfiguracija
float IMPULSA_PO_CM = 40;
float IMPULSA_PO_STUPNJU = 5.3;
int BAZNA_BRZINA = 200;
int MIN_BRZINA = 100; // Povecano jer desni motor ne radi dobro ispod 100
float Kp_Enc = 2.0; // Pojacanje P-regulatora za sinkronizaciju

void voziRavno(float cm, int speed) {
    if (cm == 0) return;

    resetirajEnkodere();
    resetirajGyro(); // Reset Gyro za pravocrtnu voznju (Target = 0)
    
    // Smjernica 2: Mjerenje udaljenosti (Max Encoder Logic)
    long cilj = abs(cm * IMPULSA_PO_CM);
    int smjer = (cm > 0) ? 1 : -1;
    
    // Parametri za Ramp-Down (Usporavanje)
    long rampThreshold = cilj * 0.2; // Zadnjih 20%
    if (rampThreshold > (15 * IMPULSA_PO_CM)) rampThreshold = 15 * IMPULSA_PO_CM; // Max 15cm usporavanja
    
    int maxBrzina = (speed > 0) ? speed : BAZNA_BRZINA; 

    unsigned long pocetak = millis();
    unsigned long zadnjiPomak = millis();
    long zadnjaPozicija = 0;
    unsigned long timeout = 10000; 

    while (true) {
        long encL = abs(dohvatiLijeviEnkoder());
        long encR = abs(dohvatiDesniEnkoder());
        
        // Smjernica 2: Koristimo MAX vrijednost enkodera za prekid
        long trenutniPut = max(encL, encR); 
        long preostalo = cilj - trenutniPut;

        // 1. Provjera kraja
        if (preostalo <= 0) break;
        
        // Safety: Emergency Stop from Dashboard
        if (provjeriHitniStop()) break;
        
        // Safety: Timeout
        if (millis() - pocetak > timeout) break;
        
        // Safety: Stall Detection
        if (abs(trenutniPut - zadnjaPozicija) > (IMPULSA_PO_CM / 2)) {
            zadnjaPozicija = trenutniPut;
            zadnjiPomak = millis();
        } else {
            if (millis() - zadnjiPomak > 1000) break;
        }
        
        // 2. Izracun Brzine (Ramp-Down Profil)
        int trenutnaBrzina = maxBrzina;
        if (preostalo < rampThreshold) {
            trenutnaBrzina = map(preostalo, 0, rampThreshold, MIN_BRZINA, maxBrzina);
        }

        // 3. Smjernica 1: Održavanje pravca isključivo preko IMU-a
        // Target kut je 0.
        // Ako robot skrene LIJEVO -> kut je NEGATIVAN (npr -5).
        // Zelimo skrenuti DESNO da se vratimo na 0.
        // Desno skretanje = Lijevi motor BRZE, Desni SPORIJE.
        
        float currentAngle = dohvatiKutGyro();
        float Kp_Gyro_Drive = 5.0; 

        // Ako je currentAngle = -5 (Lijevo) -> korekcija = -25.
        // Zelimo motL povecati -> motL = speed - korekcija (speed - (-25) = speed + 25)
        // Zelimo motR smanjiti -> motR = speed + korekcija (speed + (-25) = speed - 25)
        
        int korekcija = currentAngle * Kp_Gyro_Drive; 

        int motL = trenutnaBrzina - korekcija;
        int motR = trenutnaBrzina + korekcija;
        
        // Limitiranje PWM signala
        motL = constrain(motL, 0, 255);
        motR = constrain(motR, 0, 255);
        
        // 4. Pogon
        if (smjer > 0) {
            lijeviMotor(motL);
            desniMotor(motR);
        } else {
            lijeviMotor(-motL);
            desniMotor(-motR);
        }
        
        delay(1); 
        
        // Slanje telemetrije tijekom voznje
        azurirajIMU();
        posaljiTelemetriju();
    }
    
    // Smjernica 3: Uklanjanje poravnavanja (No End-of-Move Equalization)
    // Odmah kocimo bez ikakve dodatne logike enkodera
    motori_koci();
    delay(500); // Kratko kocenje
    stani();    // Oslobodi motor

    // Smjernica 4: Telemetrijska potvrda
    Serial.println("TELEMETRIJA: Zavrsena komanda voziRavno");
}

// Ostatak funkcija (prazne ili implementirane kasnije)
// 4. Manualna vožnja
// Ako je cm > 0, vozi dok ne prijedje put (blokirajuce).
// Ako je cm == 0, samo pokreni motore (ne blokira).
void vozi(int l, int r, float cm) {
    // 1. Pokreni motore
    ::lijeviMotor(l);
    ::desniMotor(r);

    // 2. Ako nema distance, gotovi smo (non-blocking)
    if (cm <= 0) return;

    // 3. Ako ima distance, cekamo (blocking)
    resetirajEnkodere();
    long cilj = abs(cm * IMPULSA_PO_CM);
    
    // Safety timeout? Za sad ne, vjerujemo enkoderima.
    
    while (true) {
        long encL = abs(dohvatiLijeviEnkoder());
        long encR = abs(dohvatiDesniEnkoder());
        
        // Gledamo prosjek puta
        // Gledamo prosjek puta
        if ((encL + encR) / 2 >= cilj) {
            break;
        }
        
        if (provjeriHitniStop()) break;
        
        delay(1);
    }
    
    stani();
}

void vozi(int l, int r) {
    vozi(l, r, 0);
}
// --- Funkcije za okretanje (koriste IMU/Žiroskop) ---

// Helper za ažuriranje IMU-a (ako nije dostupan preko headera)
extern void azurirajIMU(); 

void okreni(float kut, int speedArg) {
    if (kut == 0) return;
    
    // resetirajEnkodere(); // VISE NE KORISTIMO
    resetirajGyro(); // KORISTIMO GYRO - Reset na 0
    
    // 5.9375 impulsa po stupnju -> NE KORISTIMO
    // long cilj = abs(kut * IMPULSA_PO_STUPNJU);
    float ciljKut = abs(kut);

    // int speed = (speedArg > 0) ? speedArg : BAZNA_BRZINA;
    int speed = 120; // FIKSNA BRZINA ZA OKRETANJE
    
    unsigned long pocetak = millis();
    unsigned long zadnjiPomak = millis();
    // long zadnjaPozicija = 0; // Encoder position
    float zadnjiKut = 0;
    unsigned long timeout = 10000;
    
    // kut > 0 -> Desno (L naprijed, D nazad)
    // kut < 0 -> Lijevo (L nazad, D naprijed)
    int dirL = (kut > 0) ? 1 : -1;
    int dirR = (kut > 0) ? -1 : 1;

    // --- GYRO LOOP ---
    while(true) {
        float currKut = abs(dohvatiKutGyro());
        float preostalo = ciljKut - currKut;

        if (preostalo <= 0) break;
        
        // Safety: Emergency Stop
        if (provjeriHitniStop()) break;
        
        // Safety: Timeout
        if (millis() - pocetak > timeout) break;

        // Safety based on Gyro change
        if (abs(currKut - zadnjiKut) > 1.0) {
             zadnjiKut = currKut;
             zadnjiPomak = millis();
        } else {
             if (millis() - zadnjiPomak > 2000) break; // 2 sec stall timeout
        }
        
        // Ramp-Down Logic
        int trenutnaBrzina = speed;
        if (preostalo < 30.0) {
            trenutnaBrzina = map(preostalo, 0, 30, MIN_BRZINA, speed);
        }
        
        lijeviMotor(trenutnaBrzina * dirL);
        desniMotor(trenutnaBrzina * dirR);
        delay(1);
        
        azurirajIMU();
        posaljiTelemetriju();
    }
    
    // Aktivno kocenje
    motori_koci();
    delay(500);
    stani();
}

void skreni(float kut, int speedArg) {
    if (kut == 0) return;
    
    resetirajEnkodere();
    
    long cilj = abs(kut * IMPULSA_PO_STUPNJU);
    // kut > 0 -> Desno (Lijevi vozi)
    // kut < 0 -> Lijevo (Desni vozi)
    
    // int speed = (speedArg > 0) ? speedArg : BAZNA_BRZINA;
    int speed = 120; // FIKSNA BRZINA ZA SKRETANJE
    
    unsigned long pocetak = millis();
    unsigned long zadnjiPomak = millis();
    long zadnjaPozicija = 0;
    unsigned long timeout = 10000;

    while(true) {
        long encL = abs(dohvatiLijeviEnkoder());
        long encR = abs(dohvatiDesniEnkoder());
        
        // Koji enkoder pratimo?
        long trenutni = (kut > 0) ? encL : encR;
        
        if (trenutni >= cilj) break;
        
        if (provjeriHitniStop()) break;
        
        // Safety
        if (millis() - pocetak > timeout) break;
        
        if (abs(trenutni - zadnjaPozicija) > (IMPULSA_PO_STUPNJU * 5)) {
            zadnjaPozicija = trenutni;
            zadnjiPomak = millis();
        } else {
             if (millis() - zadnjiPomak > 1000) break; 
        }
        
        if (kut > 0) {
            // Desno: L vozi, R stoji
            lijeviMotor(speed);
            desniMotor(0);
        } else {
            // Lijevo: R vozi, L stoji
            lijeviMotor(0);
            desniMotor(speed);
        }
        delay(1);
        azurirajIMU();
        posaljiTelemetriju();
    }
    stani();
}
void postaviKonfigKretanja(float i) { IMPULSA_PO_CM = i; }
void stani() {
    motori_stani();
}

void zaustaviKretanje() { 
    stani(); 
}


void postaviParametre(float imp, int brzina, float kp, int min_brzina, float impDeg) {
    if (imp > 0) IMPULSA_PO_CM = imp;
    if (brzina > 0) BAZNA_BRZINA = brzina;
    if (kp >= 0) Kp_Enc = kp;
    if (min_brzina >= 0) MIN_BRZINA = min_brzina;
    if (impDeg > 0) IMPULSA_PO_STUPNJU = impDeg;
}

bool provjeriHitniStop() {
    if (Serial2.available()) {
        String msg = Serial2.readString(); // Quick read due to setTimeout(5)
        if (msg.indexOf("stop") >= 0) {
            stani();
            return true;
        }
    }
    return false;
}
