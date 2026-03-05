/**
 * Manipulator.h
 * 
 * Modul za kontrolu 6-DOF robotske ruke s hvataljkom.
 * Koristi PCA9685 PWM driver.
 * Jezik: C++ / Arduino
 */

#ifndef MANIPULATOR_H
#define MANIPULATOR_H

#include <Arduino.h>
#include "HardwareMap.h"

// --- KONFIGURACIJA I KONSTANTE ---
#define REV_SERVO_MIN 500        // Min PWM puls (REV Smart Servo)
#define REV_SERVO_MAX 2500       // Max PWM puls (REV Smart Servo)
#define SG90_SERVO_MIN 600       // Min PWM puls (9g SG90 Servo)
#define SG90_SERVO_MAX 2400      // Max PWM puls (9g SG90 Servo)
#define KORAK_MK 3.0             // Brzina kretanja (stupnjeva po koraku)

// Kanali prema RukaTest.ino
#define CH_BAZA      0
#define CH_RAME      1
#define CH_LAKAT     2
#define CH_ZGLOB     3
#define CH_HVATALJKA 4

// Pozicije (Glavne)
const int pozicijaParking[5]        = {135, 164, 40, 100, 20};
const int pozicijaSafe[5]           = {135, 110, 80, 60, 90};
const int pozicijaSafePickup[5]     = {135, 110, 80, 60, 145}; // SAFE ali hvataljka drzi objekt
const int pozicijaVoznja[5]         = {135, 140, 60, 150, 25};
const int pozicijaPripremaQR[5]     = {190, 90, 60, 50, 40};
const int pozicijaCitanjeQR[5]      = {250, 80, 80, 55, 25};
const int pozicijaPripremaPickup[5] = {135, 80, 90, 40, 80};
const int pozicijaPickup[5]         = {135, 65, 130, 70, 145}; // Pretpostavljena greška u nazivu kod korisnika za drugu PRIPREMA_PICKUP
const int pozicijaProvjeraMetal[5]  = {185, 105, 50, 70, 145};
const int pozicijaVoznjaPickup[5]   = {130, 150, 55, 20, 145};

// Pozicije za ostavljanje (D1, D2, D3)
const int pozicijaOstavljanjePriprema[5] = {135, 100, 90, 40, 145};
const int pozicijaOstavljanjeD1[5]       = {190, 80, 110, 60, 40};
const int pozicijaOstavljanjeD2[5]       = {135, 80, 110, 60, 40};
const int pozicijaOstavljanjeD3[5]       = {80, 90, 90, 90, 90};
const int pozicijaProvjeraOstavljanje[5] = {135, 90, 90, 90, 90};

// Placeholder pozicije za ostale (Korisnik će ažurirati kasnije)
const int pozicijaTrazimObjekt[5]   = {135, 90, 90, 90, 90};
const int pozicijaHvatanje[5]       = {135, 90, 90, 90, 90};
const int pozicijaSpremi[5]         = {135, 90, 90, 90, 90};

// Stanja za State Machine (Hrvatski)
enum StanjeRuke {
    STANJE_MIRUJE,
    STANJE_PARKIRANJE,
    // Sekvencijalna stanja za orkestraciju
    STANJE_SEK_PRIPREMA,  // Odlazak na poziciju iznad
    STANJE_SEK_SPUSTANJE, 
    STANJE_SEK_HVATANJE, 
    STANJE_SEK_DIZANJE,   
    STANJE_SEK_SPREMANJE,

    // Testne sekvence kretanja
    STANJE_TEST_START_CEKANJE,
    STANJE_TEST_START_SAFE,
    
    STANJE_TEST_QR_KOD,

    STANJE_TEST_NAKON_QR_VOZNJA,

    STANJE_TEST_HVATANJE_SPUSTANJE,
    STANJE_TEST_HVATANJE_SPREMANJE,
    STANJE_SAFE_PICKUP_K1,
    STANJE_SAFE_PICKUP_K2,

    // TOF Skeniranje za PROVJERA_PICKUP
    STANJE_SKEN_START,     // Baza ide na pocetnu poziciju (170°)
    STANJE_SKEN_LEVO,      // Sweepuje bazu od start -> start-30°, biljezi min TOF
    STANJE_SKEN_DESNO,     // Sweepuje bazu od start-30° -> start+30°, biljezi min TOF
    STANJE_SKEN_NAMJESTI,  // Baza ide na kut s minimalnom udaljenoscu

    // QR Cekanje i Wiggle
    STANJE_QR_CEKANJE,     // Ceka QR signal 3s
    STANJE_QR_WIGGLE_LR,   // Wiggle baze u lijevo
    STANJE_QR_WIGGLE_RL,   // Wiggle baze u desno

    // Sekvencijalno ostavljanje (Svi motori pa hvataljka)
    STANJE_DROP_SEK_K1,    // Pomakni sve osim hvataljke
    STANJE_DROP_SEK_K2     // Otvori hvataljku
};

class Manipulator {
private:
    float trenutniKutovi[5];
    float ciljaniKutovi[5];
    float brzine[5]; // Brzina za svaki servo za sinkronizirano kretanje

    StanjeRuke trenutnoStanje;
    String tipSekvence; 
    unsigned long zadnjeVrijeme;
    int zadnjiPresetIdx; // -1 ako je manualno pomaknut
    int odgodeniCiljevi[5]; // Cuva ciljnu poziciju dok ruka ide u SAFE

    // TOF Skeniranje
    float skenPocetniKut;         // Kut baze pri pocetku skeniranja
    float skenMinKut;             // Kut na kojem je bila min udaljenost
    long  skenMinUdaljenost;      // Minimalna izmjerena udaljenost
    long  (*tofFnPtr)();          // Function pointer na getter za TOF udaljenost

    // QR Wiggle
    unsigned long qrCekanjeStart; // Kad je zapocelo cekanje QR-a
    float qrPocetnaKutBaze;       // Kut baze kada je pocelo cekanje
    bool  qrPrimljeno;            // Signal je li QR primljen

    int kutU_Pulseve(int kanal, float kut);
    bool jesuLiMotoriStigli();

public:
    Manipulator();

    /**
     * Postavlja ruku u zadanu poziciju (niz od 5 kutova).
     * Izračunava brzine za sinkronizirano kretanje (kao premjestiRobot).
     */
    void postaviPoziciju(const int* poz);
    void postaviSve(const int* poz, int presetIdx = -1);

    void postaviKut(int kanal, float stupnjevi);
    float dohvatiKut(int kanal);
    
    void azuriraj();
    void zapocniSekvencu(String sekvenca);
    void parkiraj();
    void ucitajPreset(int idx);
    void pokreniTestSekvencu(int broj);
    
    int dohvatiPresetIdx();
    String dohvatiNazivPozicije();

    // --- TOF Skeniranje PRIPREMA_PICKUP ---
    void setTofFunction(long (*getTof)());
    void ucitajPreset6Skeniranje(long (*getTof)() = nullptr); // Može primiti novi ili koristiti postojeći

    // --- QR Cekanje i Wiggle ---
    void zapocniCekanjeQR();
    void primiQRSignal();
};

#endif // MANIPULATOR_H
