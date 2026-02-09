/**
 * Manipulator.cpp
 * 
 * Implementacija logike manipulatora.
 */

#include "Manipulator.h"

Manipulator::Manipulator() {
    trenutnoStanje = STANJE_MIRUJE;
    tipSekvence = "";
    zadnjeVrijeme = 0;

    // Inicijalizacija na sigurnu poziciju (npr. sredina)
    for (int i = 0; i < 7; i++) {
        trenutniKutovi[i] = 90.0;
        ciljaniKutovi[i] = 90.0;
    }
}

int Manipulator::kutU_PWM(float kut) {
    // Mapiranje 0-180 stupnjeva u PWM pulseve
    return map((long)kut, 0, 180, SERVO_MIN, SERVO_MAX);
}

void Manipulator::postaviKut(int kanal, float stupnjevi) {
    float konacniKut = stupnjevi;

    // Logika prijenosnog omjera za bazu
    if (kanal == KANAL_BAZA) {
        konacniKut = stupnjevi / OMJER_PRIJENOSA_BAZA;
    }

    // Ograničenje na raspon serva 0-180
    if (konacniKut < 0) konacniKut = 0;
    if (konacniKut > 180) konacniKut = 180;

    ciljaniKutovi[kanal] = konacniKut;
    ciljaniKutovi[kanal] = konacniKut;
}

float Manipulator::dohvatiKut(int kanal) {
    if (kanal < 0 || kanal >= 7) return 0;
    return ciljaniKutovi[kanal]; 
}

int Manipulator::dohvatiCiljaniPreset() {
    return ciljaniPresetIndex;
}

bool Manipulator::jesuLiMotoriStigli() {
    float tolerancija = 1.0; // Stupnjevi
    for (int i = 0; i < 7; i++) {
        if (abs(trenutniKutovi[i] - ciljaniKutovi[i]) > tolerancija) {
            return false;
        }
    }
    return true;
}

// Helper za primjenu preseta iz configa
void Manipulator::primjeniPreset(int idx) {
    if (idx < 0 || idx >= 15) return;
    for (int i = 0; i < 6; i++) {
        // Ignoriraj kanal hvataljke (6) ako je preset 90? Ne, preset sprema sve.
        // Ali hvataljku cesto rucno kontroliramo (otvori/zatvori).
        // Za sad primjeni SVE osim Hvataljke (Kanal 6), nju kontrolira State Machine eksplicitno.
        if (i == KANAL_HVATALJKA) continue; 
        
        postaviKut(i, config.presets[idx].angles[i]);
    }
}

void Manipulator::zapocniSekvencu(String sekvenca) {
    tipSekvence = sekvenca;
    
    // Mapiranje sekvence na ciljani Preset
    if (sekvenca == "uzmi_boca") ciljaniPresetIndex = 2;
    else if (sekvenca == "uzmi_limenka") ciljaniPresetIndex = 3;
    else if (sekvenca == "uzmi_spuzva") ciljaniPresetIndex = 4;
    else ciljaniPresetIndex = 1; // Default Safe/Voznja

    // Pocetno stanje
    trenutnoStanje = STANJE_SEK_PRIPREMA;
}

void Manipulator::postaviUVozaPoziciju() {
    // Postavljanje ruke u definiranu POZA_VOZNJA
    // Koristimo postaviKut metodu koja postavlja CILJEVE.
    // azuriraj() ce se pobrinuti za Soft-Start tranziciju.
    postaviKut(KANAL_BAZA, KUT_POZA_VOZNJA_BAZA);
    postaviKut(KANAL_RAME, KUT_POZA_VOZNJA_RAME);
    postaviKut(KANAL_LAKAT, KUT_POZA_VOZNJA_LAKAT);
    postaviKut(KANAL_ZGLOB_NAGIB, KUT_POZA_VOZNJA_ZGLOB);
    
    // Ostali zglobovi po potrebi (Rotacija sace, Hvat)
    // Neka rotacija sake bude ravna (90)
    postaviKut(KANAL_ZGLOB_ROT, 90.0);
    // Hvat neka ostane kakav je ili ga zatvorimo?
    // Bolje zatvoriti da ne zapinje.
    postaviKut(KANAL_HVATALJKA, 90.0); 

    // Resetiraj State Machine da ne ometa
    trenutnoStanje = STANJE_MIRUJE;
}

void Manipulator::azuriraj() {
    // --- Soft-Start Logika (Time Based) ---
    // Neka se osvježava svakih 20ms (50Hz) za fluidnost
    unsigned long trenutnoVrijeme = millis();
    if (trenutnoVrijeme - zadnjeVrijeme < 20) {
        return; 
    }
    zadnjeVrijeme = trenutnoVrijeme;

    bool kretanjeAct = false;
    for (int i = 0; i < 7; i++) {
        // Primjena Soft-Starta
        if (abs(trenutniKutovi[i] - ciljaniKutovi[i]) > KORAK_MK) {
            if (trenutniKutovi[i] < ciljaniKutovi[i]) {
                trenutniKutovi[i] += KORAK_MK;
            } else {
                trenutniKutovi[i] -= KORAK_MK;
            }
            kretanjeAct = true;
        } else {
            // Dovoljno blizu, postavi na cilj
            trenutniKutovi[i] = ciljaniKutovi[i];
        }
        
        // Slanje PWM-a
        pwm.setPWM(i, 0, kutU_PWM(trenutniKutovi[i]));
    }
    
    // Ako se krecemo, ne radimo State Machine prijelaze unutar istog tika
    // zapravo zelimo paralelno, state machine samo mijenja ciljeve. OK.
    
    // --- State Machine Logika ---
    // Provjeravamo je li ruka stigla na cilj PRIJE nego mijenjamo stanje
    if (kretanjeAct) return; 

    switch (trenutnoStanje) {
        case STANJE_MIRUJE:
            break;

        case STANJE_DIZANJE_SIGURNO: // Legacy
            // Legacy hardcoded fallback
            postaviKut(KANAL_RAME, 130); 
            postaviKut(KANAL_LAKAT, 90);
            trenutnoStanje = STANJE_ROTACIJA_BAZE;
            break;

        // --- NOVA SEKVENCIJALNA LOGIKA ---
        case STANJE_SEK_PRIPREMA:
            // 1. Otvori hvataljku
            postaviKut(KANAL_HVATALJKA, 20);
            // 2. Odi u sigurnu poziciju (Preset 1 - Voznja) da ne udaris nista dok se okreces
            // Ili koristi "Parkiraj" (0). Koristimo 1 (Voznja/Safe).
            primjeniPreset(1); 
            // Ali zelimo rotirati BAZU prema CILJU odmah? 
            // Ne, prvo digni, pa onda u sljedecem koraku rotiraj/spusti.
            trenutnoStanje = STANJE_SEK_SPUSTANJE;
            break;

        case STANJE_SEK_SPUSTANJE:
            // Odi na ciljanu poziciju (npr. UzmiBoca - Preset 2)
            primjeniPreset(ciljaniPresetIndex);
            // Ovdje se ruka spusta i rotira prema objektu istovremeno.
            // SoftStart osigurava da je fluidno.
            trenutnoStanje = STANJE_SEK_HVATANJE;
            break;

        case STANJE_SEK_HVATANJE:
            // Stigli smo dolje. Zatvori hvataljku.
            postaviKut(KANAL_HVATALJKA, 90); // Zatvoreno
            // Ovdje bi dobro dosao mali delay da se stigne zatvoriti prije dizanja.
            // Budući da 'azuriraj' ne blokira, delay se rjesava timerom ili
            // jednostavnim hackom: ako je ruka jos u pokretu (hvataljka), necemo doci ovdje.
            // Ali hvataljka ima malu putanju. 
            // Za sada odmah prelazimo na dizanje.
            trenutnoStanje = STANJE_SEK_DIZANJE;
            break;

        case STANJE_SEK_DIZANJE:
            // Digni na sigurnu visinu (Voznja - Preset 1) s predmetom
            primjeniPreset(1);
            trenutnoStanje = STANJE_SEK_SPREMANJE;
            break;

        case STANJE_SEK_SPREMANJE:
            // Odi do spremnika (Preset 5, 6 ili 7)
            // Moramo znati u koji spremnik ide.
            // Logiku mapiranja (Boca->Spremi1) radimo u zapocniSekvencu.
            // Ovdje koristimo pomocnu varijablu ili hardcodiramo?
            // Za sad cemo koristiti `ciljaniPresetIndex` koji cemo prenamijeniti
            // ili dodati `destinacijaPresetIndex`.
            // Pojednostavljenje: Vratit cemo se u "Voznja" i stati.
            // User trazi orkestraciju hvatanja. Spremanje je drugi par rukava.
            // Ali ajde dovrsimo ciklus.
            // Pretpostavimo da je cilj parkiranje (Preset 0).
            primjeniPreset(0); 
            // Kad stigne, otvori hvataljku ako treba?
            // Za sad stajemo u STANJE_MIRUJE.
            trenutnoStanje = STANJE_MIRUJE;
            break;

        case STANJE_ROTACIJA_BAZE: // Legacy (zadrzavamo da ne pukne compiler ako se negdje poziva)
            trenutnoStanje = STANJE_MIRUJE; 
            break;
        case STANJE_SPUSTANJE: 
            trenutnoStanje = STANJE_MIRUJE; 
            break;
        case STANJE_ISPUSTANJE: 
            trenutnoStanje = STANJE_MIRUJE; 
            break;
        case STANJE_POVRATAK: 
            trenutnoStanje = STANJE_MIRUJE; 
            break;
        // case STANJE_HVAITANJE_S_KROVA: 
        //      trenutnoStanje = STANJE_SEK_PRIPREMA; // Redirekcija
        //      break;
    }
}
