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
#define OMJER_PRIJENOSA_BAZA 2.0 // Omjer zupčanika na bazi (2:1)
#define SERVO_MIN 150            // Min PWM puls (cca 0 stupnjeva)
#define SERVO_MAX 600            // Max PWM puls (cca 180 stupnjeva)
#define KORAK_MK 1.0             // Korak pomaka (Soft-Start brzina)

// Kutovi za spremanje predmeta (Krov & Odlaganje)
#define KUT_KROV_SLOT_1    10.0   // Slot 1 na krovu
#define KUT_KROV_SLOT_2    170.0  // Slot 2 na krovu
#define KUT_ODLAGANJE_D1   45.0   // Lijevi spremnik
#define KUT_ODLAGANJE_D2   90.0   // Srednji spremnik
#define KUT_ODLAGANJE_D3   135.0  // Desni spremnik

// Poza za vožnju (Kamera gleda dolje pod 45 stupnjeva)
#define KUT_POZA_VOZNJA_RAME  135.0
#define KUT_POZA_VOZNJA_LAKAT 90.0
#define KUT_POZA_VOZNJA_ZGLOB 135.0 // Kompenzacija za 45 stupnjeva
#define KUT_POZA_VOZNJA_BAZA  90.0  // Ravno naprijed

// Stanja za State Machine (Hrvatski)
enum StanjeRuke {
    STANJE_MIRUJE,
    STANJE_DIZANJE_SIGURNO,
    STANJE_ROTACIJA_BAZE,
    STANJE_SPUSTANJE,
    STANJE_ISPUSTANJE,
    STANJE_POVRATAK,
    // Sekvencijalna stanja za orkestraciju
    STANJE_SEK_PRIPREMA, // Odlazak na poziciju iznad (Rame/Lakat)
    STANJE_SEK_SPUSTANJE, // Spustanje Z-osi (Rame/Lakat dolje)
    STANJE_SEK_HVATANJE, // Zatvaranje hvataljke
    STANJE_SEK_DIZANJE,   // Dizanje gore
    STANJE_SEK_SPREMANJE  // Rotacija u spremnik
};

class Manipulator {
private:
    // Trenutni kutovi (stvarna pozicija serva)
    float trenutniKutovi[7];
    // Ciljani kutovi (željena pozicija)
    float ciljaniKutovi[7];

    // State Machine varijable
    StanjeRuke trenutnoStanje;
    String tipSekvence; 
    int ciljaniPresetIndex;
    unsigned long zadnjeVrijeme; // Za tajming ako zatreba

    /**
     * Pomoćna funkcija za mapiranje kuta u PWM vrijednost.
     */
    int kutU_PWM(float kut);

    /**
     * Provjerava jesu li svi motori stigli na cilj.
     */
    bool jesuLiMotoriStigli();

    void primjeniPreset(int idx);

public:
    Manipulator();

    /**
     * Postavlja ciljani kut za određeni kanal.
     * Implementira logiku prijenosnog omjera za bazu.
     */
    void postaviKut(int kanal, float stupnjevi);
    
    /**
     * Vraća trenutni kut serva.
     */
    float dohvatiKut(int kanal);
    
    int dohvatiCiljaniPreset();

    /**
     * Glavna petlja za ažuriranje.
     * Pozivati u loop().
     * Odrađuje Soft-Start (postepeno pomicanje) i State Machine.
     */
    void azuriraj();

    /**
     * Započinje sekvencu (spremanje ili odlaganje).
     * @param sekvenca - npr. "krov1", "odlozi_d1"
     */
    void zapocniSekvencu(String sekvenca);

    /**
     * Postavlja ruku u stabilnu pozu za vožnju.
     * Kamera gleda ispred robota (45°).
     * Koristi Soft-Start.
     */
    void postaviUVozaPoziciju();
};

#endif // MANIPULATOR_H
