/**
 * Kretanje.cpp
 * 
 * Implementacija Non-Blocking State Machine za kretanje.
 */

#include "Kretanje.h"
#include "HardwareMap.h"
#include "Motori.h"
#include "HardwareMap.h"
#include "Motori.h"
// #include "SenzoriLinije.h" // Uklonjeno
#include "Enkoderi.h"
#include "IMU.h"
#include "Vision.h"
#include "Enkoderi.h"
#include "IMU.h"


// --- PID Parametri (i Fusion koeficijenti) ---
float Kp = 35.0; // Zadržano za kompatibilnost ako zatreba
float Ki = 0.0;
float Kd = 15.0;
int baznaBrzina = 100;

// Fusion paramteri (Tuneable)
float K_VISION = 80.0;      // Gain za kameru (Error -1.0 do 1.0 -> Max steering 80)
float K_LANE_ASSIST = 0.15; // Gain za analogne IR (Diff cca 800 -> 800 * 0.15 = 120 steering)

float proslaGreska = 0;
float integral = 0;

// --- State Machine Varijable ---
StanjeKretanja trenutnoStanjeKretanja = KRETANJE_IDLE;
float ciljnaVrijednost = 0; // CM ili Stupnjevi
float pocetnaOrijentacija = 0;
long pocetniEncL = 0;
long pocetniEncR = 0;

// Kalibracija
float IMPULSA_PO_CM = 40.0;
int ciljniSpeedL = 0;
int ciljniSpeedR = 0; 

void postaviKonfigKretanja(float impulsaPoCm) {
    IMPULSA_PO_CM = impulsaPoCm;
}

// Pomoćna za normalizaciju kuta
float normalizirajKut(float kut) {
    while (kut > 180) kut -= 360;
    while (kut < -180) kut += 360;
    return kut;
}

void zapocniVoznju(float cm) {
    resetirajEnkodere();
    azurirajIMU();
    
    pocetnaOrijentacija = dohvatiYaw();
    pocetniEncL = 0; // Resetirano
    pocetniEncR = 0; // Resetirano
    
    ciljnaVrijednost = cm * IMPULSA_PO_CM;
    
    trenutnoStanjeKretanja = KRETANJE_RAVNO;
}

void zapocniRotaciju(float stupnjevi) {
    stani();
    azurirajIMU(); // Osvježi stanje
    delay(10);     // Kratka pauza za stabilizaciju
    azurirajIMU();
    
    pocetnaOrijentacija = dohvatiYaw();
    ciljnaVrijednost = pocetnaOrijentacija + stupnjevi;
    
    // Normalizacija cilja u 0-360 range (dohvatiYaw vraca obicno 0-360 ili -180/180 ovisno o libu, ovdje pretpostavljamo dohvatiYaw vraca 0-360 iz IMU.cpp koda sto sam vidio prije, al IMU libovi cesto saraju. Prilagodit cemo se wrap-aroundu)
    
    // Ako zelimo rotirati za +90, a trenutno je 350, cilj je 440 -> 80.
    
    trenutnoStanjeKretanja = KRETANJE_ROTACIJA;
}

void straightDrive(float cm) {
    zapocniVoznju(cm);
}

void pivotTurn(float kut) {
    stani();
    azurirajIMU();
    delay(10);
    azurirajIMU(); // Double check
    
    pocetnaOrijentacija = dohvatiYaw();
    // Cilj je +kut od trenutnog
    // Ovdje ne racunamo apsolutni cilj, nego cemo u loopu racunat diff
    // Zapravo konzistentnije je apsolutni cilj.
    ciljnaVrijednost = pocetnaOrijentacija + kut;
    
    trenutnoStanjeKretanja = KRETANJE_PIVOT;
}

void differentialDrive(int l, int r, float dist) {
    resetirajEnkodere();
    ciljniSpeedL = l;
    ciljniSpeedR = r;
    ciljnaVrijednost = dist * IMPULSA_PO_CM;
    
    trenutnoStanjeKretanja = KRETANJE_DUAL;
    
    lijeviMotor(l);
    desniMotor(r);
}

void pokreniPracenjeLinije() {
    trenutnoStanjeKretanja = KRETANJE_LINIJA;
    proslaGreska = 0;
    integral = 0;
}

void zaustaviKretanje() {
    trenutnoStanjeKretanja = KRETANJE_IDLE;
    stani();
}

float dohvatiPredjeniPutCm() {
    long avgEnc = (abs(dohvatiLijeviEnkoder()) + abs(dohvatiDesniEnkoder())) / 2;
    if (IMPULSA_PO_CM == 0) return 0;
    return (float)avgEnc / IMPULSA_PO_CM;
}

bool jeUPokretu() {
    return trenutnoStanjeKretanja != KRETANJE_IDLE;
}

void azurirajKretanje() {
    if (trenutnoStanjeKretanja == KRETANJE_IDLE) {
        return;
    }

    if (trenutnoStanjeKretanja == KRETANJE_LINIJA) {
        // --- LOGIKA: Weighted Sensor Fusion ---
        
        // 1. Vision Input (-1.0 do 1.0)
        float error_vision = dohvatiVisionError();
        
        // 2. Analog Lane Assist Input (0-1023)
        float val_left = analogRead(PIN_IR_LIJEVI);
        float val_right = analogRead(PIN_IR_DESNI);
        
        // Analog feedback creates a "virtual spring" effect to center the robot.
        // Ako lijevi vidi crno (veća vrijednost), gura desno (pozitivni steering).
        // Ako desni vidi crno, gura lijevo.
        // Pretpostavka: Crno > 800 (refleksija mala?), Bijelo < 100
        // Napomena: IR senzori često daju manju vrijednost za bijelo (refleksija).
        // Ovisi o spoju (pull-up/down).
        // Prompt kaze: "If Left sees dark, push Right."
        // Neka je steering > 0 skretanje udesno (lijevi motor brzi).
        
        float error_lane = (val_left - val_right) * K_LANE_ASSIST;
        
        // 3. Final Steering
        float steering = (error_vision * K_VISION) + error_lane;
        
        // Debug
        // Serial.print("V:"); Serial.print(error_vision);
        // Serial.print(" L:"); Serial.print(val_left);
        // Serial.print(" R:"); Serial.print(val_right);
        // Serial.print(" S:"); Serial.println(steering);

        int brzinaLijevi = baznaBrzina + steering;
        int brzinaDesni = baznaBrzina - steering;
        
        brzinaLijevi = constrain(brzinaLijevi, -255, 255);
        brzinaDesni = constrain(brzinaDesni, -255, 255);
        
        lijeviMotor(brzinaLijevi);
        desniMotor(brzinaDesni);
    }
    else if (trenutnoStanjeKretanja == KRETANJE_RAVNO) {
        // --- LOGIKA ZA RAVNO ---
        long l = abs(dohvatiLijeviEnkoder());
        long r = abs(dohvatiDesniEnkoder());
        
        // Provjera kraja
        if (l >= abs(ciljnaVrijednost) || r >= abs(ciljnaVrijednost)) {
            zaustaviKretanje();
            return;
        }
        
        // Korekcija smjera
        azurirajIMU();
        float trenutniYaw = dohvatiYaw();
        float greskaSmjera = normalizirajKut(trenutniYaw - pocetnaOrijentacija);
        
        float Kp_smjer = 5.0;
        int korekcija = greskaSmjera * Kp_smjer;
        
        // Ako idemo u rikverc (cilj < 0), logika motora se mjenja, ali ovdje pretpostavimo forward ako je cm > 0
        // Za sad supportamo samo naprijed s ovim kodom, za rikverc bi trebalo paziti na znak bazneBrzine
        
        lijeviMotor(baznaBrzina + korekcija);
        desniMotor(baznaBrzina - korekcija);
    }
    else if (trenutnoStanjeKretanja == KRETANJE_ROTACIJA) {
        // --- LOGIKA ZA ROTACIJU ---
        azurirajIMU();
        float trenutniYaw = dohvatiYaw();
        
        // Izračunaj diff do cilja uzimajući u obzir wrap-around
        float diff = ciljnaVrijednost - trenutniYaw;
        // Normaliziraj diff na -180 do 180 (najkraći put)
        diff = normalizirajKut(diff);
        
        // Tolerancija
        if (abs(diff) < 2.0) {
            zaustaviKretanje();
            return;
        }
        
        int brzinaOkreta = 120;
        // Povećaj brzinu ako je daleko, smanji ako je blizu (P regulator za okret)
        // Ali pazi na minimalnu brzinu da motori ne stanu
        
        if (diff > 0) {
             // Treba ic desno (povecati kut)
             lijeviMotor(brzinaOkreta);
             desniMotor(-brzinaOkreta);
        } else {
             // Treba ic lijevo (smanjiti kut)
             lijeviMotor(-brzinaOkreta);
             desniMotor(brzinaOkreta);
        }
    }
    else if (trenutnoStanjeKretanja == KRETANJE_PIVOT) {
        azurirajIMU();
        float trenutniYaw = dohvatiYaw();
        float diff = normalizirajKut(ciljnaVrijednost - trenutniYaw);
        
        if (abs(diff) < 2.0) {
            zaustaviKretanje();
            return;
        }
        
        int spd = 120; // Fiksna brzina za pivot
        
        // Pozitivan kut -> Desno -> Lijevi motor radi, desni stoji
        if (diff > 0) {
            lijeviMotor(spd);
            desniMotor(0);
        } else {
            // Negativan kut -> Lijevo -> Desni motor radi, lijevi stoji
            lijeviMotor(0);
            desniMotor(spd);
        }
    }
    else if (trenutnoStanjeKretanja == KRETANJE_DUAL) {
        long l = abs(dohvatiLijeviEnkoder());
        long r = abs(dohvatiDesniEnkoder());
        
        // Zaustavi kad BILO KOJI dosegne dist (zapravo prompt kaze "kada brži motor prijeđe").
        // "brži motor" će prijeći veću put, pa on prije dođe do limita.
        // Ergo: max(l, r) >= limit.
        
        if (l >= abs(ciljnaVrijednost) || r >= abs(ciljnaVrijednost)) {
            zaustaviKretanje();
            return;
        }
        
        // Održavaj brzine (open loop)
        lijeviMotor(ciljniSpeedL);
        desniMotor(ciljniSpeedR);
    }

    // --- LANE ASSIST (Sigurnosni Override) ---
    // Uklonjeno jer je sada integrirano u glavnu logiku 'Weighted Sensor Fusion'
    // dok smo u modu KRETANJE_LINIJA.
    // Ako smo u KRETANJE_RAVNO ("Blind Drive"), mozda zelimo override?
    
    // Za "Blind Drive" (KRETANJE_RAVNO), ako naidjemo na liniju slucajno?
    // Prompt: "Obstacle Strategy: Blind Drive using IMU + Encoders"
    // Lane Assist je definiran kao dio Navigacije.
    // Opcionalno mozemo dodati sigurnost ovdje ako vozilo izleti sa staze tijekom "Ravno".
    // Ali s obzirom na prompt, Lane Assist je primarni nacin vodjenja (u Fusionu).
    // Ostavljamo prazno da ne smeta IMU voznji.
}
