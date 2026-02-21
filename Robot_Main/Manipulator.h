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
#define SERVO_MIN 500            // Min PWM puls (500us)
#define SERVO_MAX 2500           // Max PWM puls (2500us)
#define KORAK_MK 1.5             // Brzina kretanja (stupnjeva po koraku)

// Kanali prema RukaTest.ino
#define CH_BAZA      0
#define CH_RAME      1
#define CH_LAKAT     2
#define CH_ZGLOB     3
#define CH_HVATALJKA 4

// Pozicije
const int pozicijaParking[5] = {135, 145, 25, 90, 90};

// Stanja za State Machine (Hrvatski)
enum StanjeRuke {
    STANJE_MIRUJE,
    STANJE_PARKIRANJE,
    // Sekvencijalna stanja za orkestraciju
    STANJE_SEK_PRIPREMA,  // Odlazak na poziciju iznad
    STANJE_SEK_SPUSTANJE, 
    STANJE_SEK_HVATANJE, 
    STANJE_SEK_DIZANJE,   
    STANJE_SEK_SPREMANJE  
};

class Manipulator {
private:
    float trenutniKutovi[5];
    float ciljaniKutovi[5];
    float brzine[5]; // Brzina za svaki servo za sinkronizirano kretanje

    StanjeRuke trenutnoStanje;
    String tipSekvence; 
    unsigned long zadnjeVrijeme;

    int kutU_Pulseve(int kanal, float kut);
    bool jesuLiMotoriStigli();

public:
    Manipulator();

    /**
     * Postavlja ruku u zadanu poziciju (niz od 5 kutova).
     * Izračunava brzine za sinkronizirano kretanje (kao premjestiRobot).
     */
    void postaviPoziciju(const int poz[5]);

    void postaviKut(int kanal, float stupnjevi);
    float dohvatiKut(int kanal);
    
    void azuriraj();
    void zapocniSekvencu(String sekvenca);
    void parkiraj();
};

#endif // MANIPULATOR_H
