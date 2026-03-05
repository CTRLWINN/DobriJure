#include "Manipulator.h"

Manipulator::Manipulator() {
    trenutnoStanje = STANJE_MIRUJE;
    tipSekvence = "";
    zadnjeVrijeme = 0;

    // Inicijalizacija na parking poziciju
    zadnjiPresetIdx = 0; // 0 je Parkiraj
    for (int i = 0; i < 5; i++) {
        trenutniKutovi[i] = (float)pozicijaParking[i];
        ciljaniKutovi[i] = (float)pozicijaParking[i];
        brzine[i] = 0;
    }
}

int Manipulator::kutU_Pulseve(int kanal, float kut) {
    int maxStupnjeva = 180;
    if (kanal == CH_BAZA) maxStupnjeva = 270;

    // Ograničavanje unosa za sigurnost
    if (kut > maxStupnjeva) kut = maxStupnjeva;
    if (kut < 0) kut = 0;

    // Različiti rasponi ovisno o motoru
    if (kanal == CH_ZGLOB || kanal == CH_HVATALJKA) {
        return map((long)kut, 0, maxStupnjeva, SG90_SERVO_MIN, SG90_SERVO_MAX);
    } else {
        return map((long)kut, 0, maxStupnjeva, REV_SERVO_MIN, REV_SERVO_MAX);
    }
}

void Manipulator::postaviPoziciju(const int* poz) {
    float maxDelta = 0;
    for (int i = 0; i < 5; i++) {
        float d = abs(poz[i] - trenutniKutovi[i]);
        if (d > maxDelta) maxDelta = d;
    }

    // Određujemo broj koraka temeljem najvećeg pomaka da zadržimo konstantnu brzinu
    // 0.8 stupnjeva po koraku (pri 20ms refreshu) je brzina koju ste tražili.
    float brKoraka = maxDelta / 0.8; 
    if (brKoraka < 1.0) brKoraka = 1.0; 

    for (int i = 0; i < 5; i++) {
        ciljaniKutovi[i] = poz[i];
        brzine[i] = (ciljaniKutovi[i] - trenutniKutovi[i]) / brKoraka;
    }
}

void Manipulator::postaviKut(int kanal, float stupnjevi) {
    if (kanal < 0 || kanal >= 5) return;
    
    // Tijekom učenja (manualnog postavljanja kuteva putem klizača),
    // onemogućavamo SAFE proceduru kako bi korisnik imao direktnu kontrolu,
    // a kako se ostali motori ne bi nasumično kretali u SAFE položaj.

    ciljaniKutovi[kanal] = stupnjevi;
    zadnjiPresetIdx = -1; // Manualna promjena
    // Za pojedinačni kut koristimo default brzinu, a za hvataljku duplo
    float korak = (kanal == CH_HVATALJKA) ? (KORAK_MK * 2.0) : KORAK_MK;
    brzine[kanal] = (stupnjevi > trenutniKutovi[kanal]) ? korak : -korak;
}

float Manipulator::dohvatiKut(int kanal) {
    if (kanal < 0 || kanal >= 5) return 0;
    return trenutniKutovi[kanal]; 
}

bool Manipulator::jesuLiMotoriStigli() {
    float tolerancija = 0.5;
    for (int i = 0; i < 5; i++) {
        if (abs(trenutniKutovi[i] - ciljaniKutovi[i]) > tolerancija) {
            return false;
        }
    }
    return true;
}

void Manipulator::parkiraj() {
    postaviSve(pozicijaParking, 0);
}

void Manipulator::postaviSve(const int* poz, int presetIdx) {
    postaviPoziciju(poz);
    zadnjiPresetIdx = presetIdx;
    trenutnoStanje = STANJE_MIRUJE;
}

void Manipulator::ucitajPreset(int idx) {
    if (idx < 0 || idx >= 15) return;
    
    // Budući da ne koristimo EEPROM po želji korisnika, 
    // mapiramo presete na hardkodirane konstante
    const int* cilj = NULL;
    switch(idx) {
        case 0:  cilj = pozicijaParking; break;
        case 1:  cilj = pozicijaSafe; break;
        case 2:  cilj = pozicijaVoznja; break;
        case 3:  cilj = pozicijaPripremaQR; break;
        case 4:  cilj = pozicijaCitanjeQR; break;
        case 5:  cilj = pozicijaPripremaPickup; break;
        case 6:  cilj = pozicijaSafe; break; // PROVJERA_PICKUP (nije definirana)
        case 7:  cilj = pozicijaPickup; break;
        case 8:  cilj = pozicijaProvjeraMetal; break;
        case 9:  cilj = pozicijaVoznjaPickup; break;
        case 10: cilj = pozicijaOstavljanjePriprema; break;
        case 11: cilj = pozicijaOstavljanjeD1; break;
        case 12: cilj = pozicijaOstavljanjeD2; break;
        case 13: cilj = pozicijaOstavljanjeD3; break;
        case 14: cilj = pozicijaProvjeraOstavljanje; break;
        default: cilj = pozicijaSafe; break;
    }

    postaviSve(cilj, idx);
}

int Manipulator::dohvatiPresetIdx() {
    return zadnjiPresetIdx;
}

String Manipulator::dohvatiNazivPozicije() {
    if (zadnjiPresetIdx == -1) return "Manual";
    if (trenutnoStanje == STANJE_PARKIRANJE) return "Parking";
    if (trenutnoStanje == STANJE_TEST_START_SAFE) return "Safe";
        
    if (trenutnoStanje >= STANJE_TEST_START_CEKANJE) return "Testna Sekvenca";

    if (zadnjiPresetIdx >= 0 && zadnjiPresetIdx < 15) {
        return String(presetNames[zadnjiPresetIdx]);
    }
    return "Nepoznato";
}

void Manipulator::pokreniTestSekvencu(int broj) {
    zadnjiPresetIdx = -1;
    switch(broj) {
        case 1:
            // START PATTERN: Polako u PARKING -> Cekanje 2s -> SAFE
            postaviPoziciju(pozicijaParking);
            // Smanjimo brzinu namjerno za polagani start
            for(int i=0; i<5; i++) brzine[i] = (pozicijaParking[i] - trenutniKutovi[i]) / 300.0;
            trenutnoStanje = STANJE_TEST_START_CEKANJE;
            break;
            
        case 2:
            // Čitanje QR koda: SAFE -> QR KOD 
            postaviPoziciju(pozicijaSafe);
            trenutnoStanje = STANJE_TEST_QR_KOD;
            break;
            
        case 3:
            // QR kod očitan: (Trenutno smo u QR) -> SAFE -> VOŽNJA
            postaviPoziciju(pozicijaSafe);
            trenutnoStanje = STANJE_TEST_NAKON_QR_VOZNJA;
            break;
            
        case 4:
            // Hvatanje objekta: TRAZIM_OBJEKT -> HVATAJ -> SPREMI
            postaviPoziciju(pozicijaTrazimObjekt);
            trenutnoStanje = STANJE_TEST_HVATANJE_SPUSTANJE;
            break;
    }
}

void Manipulator::zapocniSekvencu(String sekvenca) {
    tipSekvence = sekvenca;
    trenutnoStanje = STANJE_SEK_PRIPREMA;
}

void Manipulator::azuriraj() {
    unsigned long trenutnoVrijeme = millis();
    if (trenutnoVrijeme - zadnjeVrijeme < 20) {
        return; 
    }
    zadnjeVrijeme = trenutnoVrijeme;

    bool kretanjeAct = false;
    for (int i = 0; i < 5; i++) {
        float razlika = ciljaniKutovi[i] - trenutniKutovi[i];
        
        if (abs(razlika) > abs(brzine[i]) && abs(brzine[i]) > 0.0001) {
            trenutniKutovi[i] += brzine[i];
            kretanjeAct = true;
        } else {
            // Ako je preostala razlika manja od jednog koraka, "zaključaj" na cilj
            trenutniKutovi[i] = ciljaniKutovi[i];
        }
        
        // Slanje mikrosekundi na PCA9685
        pwm.writeMicroseconds(i, kutU_Pulseve(i, trenutniKutovi[i]));
    }
    
    if (kretanjeAct) return; 

    // State Machine
    switch (trenutnoStanje) {
        case STANJE_MIRUJE:
        case STANJE_PARKIRANJE:
            if (!kretanjeAct) trenutnoStanje = STANJE_MIRUJE;
            break;

        case STANJE_SEK_PRIPREMA:
            // Primjer sekvence: Digni se prije micanja baze
            // Ovo ćemo proširiti kako budemo definirali konkretne pokrete
            trenutnoStanje = STANJE_MIRUJE;
            break;

        // --- TESTNE SEKVENCE KRETANJA ---
        
        // 1. START PATTERN: PARKING -> CEKANJE 2s -> SAFE
        case STANJE_TEST_START_CEKANJE:
            if (!kretanjeAct) {
                // Ako smo stigli u parking, cekaj 2 sekunde
                if (trenutnoVrijeme - zadnjeVrijeme > 2000) {
                    postaviPoziciju(pozicijaSafe);
                    trenutnoStanje = STANJE_TEST_START_SAFE;
                }
            } else {
                zadnjeVrijeme = trenutnoVrijeme; // Resetiraj tajmer dok se krece
            }
            break;
            
        case STANJE_TEST_START_SAFE:
            if (!kretanjeAct) trenutnoStanje = STANJE_MIRUJE;
            break;

        // 2. Čitanje QR koda: SAFE -> QR
        case STANJE_TEST_QR_KOD:
            if (!kretanjeAct) {
                postaviPoziciju(pozicijaCitanjeQR);
                trenutnoStanje = STANJE_MIRUJE; // Zavrsna destinacija
            }
            break;

        // 3. QR Očitan: QR KOD -> SAFE (odradjeno prije) -> VOŽNJA
        case STANJE_TEST_NAKON_QR_VOZNJA:
            if (!kretanjeAct) {
                postaviPoziciju(pozicijaVoznja);
                trenutnoStanje = STANJE_MIRUJE;
            }
            break;

        // 4. Hvatanje objekta: TRAZIM_OBJEKT -> HVATAJ -> SPREMI
        case STANJE_TEST_HVATANJE_SPUSTANJE:
            if (!kretanjeAct) {
                postaviPoziciju(pozicijaHvatanje);
                trenutnoStanje = STANJE_TEST_HVATANJE_SPREMANJE;
            }
            break;
            
        case STANJE_TEST_HVATANJE_SPREMANJE:
            if (!kretanjeAct) {
                postaviPoziciju(pozicijaSpremi);
                trenutnoStanje = STANJE_MIRUJE;
            }
            break;

        default:
            break;
    }
}
