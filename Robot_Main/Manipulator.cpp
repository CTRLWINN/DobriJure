#include "Manipulator.h"

Manipulator::Manipulator() {
    trenutnoStanje = STANJE_MIRUJE;
    tipSekvence = "";
    zadnjeVrijeme = 0;
    tofFnPtr = nullptr;
    skenPocetniKut = 135.0;
    skenMinKut = 135.0;
    skenMinUdaljenost = 9999;
    qrCekanjeStart = 0;
    qrPocetnaKutBaze = 135.0;
    qrPrimljeno = false;

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
    float brKoraka = maxDelta / 4.0; 
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
    if (idx < 0 || idx >= 16) return;
    
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
        case 6: 
            // PROVJERA_PICKUP - Okida skeniranje ako je TOF povezan
            if (tofFnPtr != nullptr) {
                ucitajPreset6Skeniranje(); // Pokreće scanning sweep
                return;
            }
            cilj = pozicijaProvjeraMetal; 
            break;
        case 7:  cilj = pozicijaPickup; break;
        case 8:  cilj = pozicijaProvjeraMetal; break;
        case 9:  cilj = pozicijaVoznjaPickup; break;
        case 10: cilj = pozicijaOstavljanjePriprema; break;
        case 11: 
        case 12: 
        case 13: 
            // Sekvencijalna dostava (Svi motori pa hvataljka na kraju)
            if (idx == 11) cilj = pozicijaOstavljanjeD1;
            else if (idx == 12) cilj = pozicijaOstavljanjeD2;
            else cilj = pozicijaOstavljanjeD3;

            for(int i=0; i<5; i++) odgodeniCiljevi[i] = cilj[i];
            
            int tmpDrop[5];
            for(int i=0; i<4; i++) tmpDrop[i] = cilj[i]; // Baza, Rame, Lakat, Zglob na cilj
            tmpDrop[CH_HVATALJKA] = (int)trenutniKutovi[CH_HVATALJKA]; // Hvataljka ostaje zatvorena/gdje je bila
            
            postaviPoziciju(tmpDrop);
            zadnjiPresetIdx = idx;
            trenutnoStanje = STANJE_DROP_SEK_K1;
            return; 

        case 14: cilj = pozicijaProvjeraOstavljanje; break;
        case 15: 
            // Inicira sekvencirani dolazak (Rame+Zglob pa onda Lakat)
            // Stoga ne postavljamo odmah 'cilj' preko postaviSve()
            zadnjiPresetIdx = 15;
            
            // Postavi Rame (CH_RAME=1) i Zglob (CH_ZGLOB=3) na cilj
            int tmpKutovi[5];
            for (int i=0; i<5; i++) tmpKutovi[i] = (int)trenutniKutovi[i]; // Drži gdje jesi
            tmpKutovi[CH_RAME] = pozicijaSafePickup[CH_RAME];
            tmpKutovi[CH_ZGLOB] = pozicijaSafePickup[CH_ZGLOB];
            postaviPoziciju(tmpKutovi); // Generira brzine kretanja
            
            trenutnoStanje = STANJE_SAFE_PICKUP_K1;
            return; // Prekida switch i ne zove postaviSve
        default: cilj = pozicijaSafe; break;
    }

    postaviSve(cilj, idx);
}

void Manipulator::setTofFunction(long (*getTof)()) {
    tofFnPtr = getTof;
}

void Manipulator::ucitajPreset6Skeniranje(long (*getTof)()) {
    if (getTof != nullptr) tofFnPtr = getTof;
    if (tofFnPtr == nullptr) return; // Ne skeniraj ako nemas funkciju
    
    // Skeniranje krece od 170 prema 100 stupnjeva
    skenPocetniKut = 170.0f; 
    skenMinUdaljenost = 9999;
    skenMinKut = skenPocetniKut;

    // Postavi zglobove na poziciju za PRIPREMA_PICKUP (to je visina za skeniranje prema uputi)
    postaviSve(pozicijaPripremaPickup, 6); 

    // Inicijalna pozicija baze za pocetak skeniranja
    int tmpKutovi[5];
    for (int i=0; i<5; i++) tmpKutovi[i] = pozicijaPripremaPickup[i];
    tmpKutovi[CH_BAZA] = (int)skenPocetniKut;
    postaviPoziciju(tmpKutovi);
    
    zadnjiPresetIdx = 6; 
    trenutnoStanje = STANJE_SKEN_LEVO; // Koristimo LEVO za sweep 170 -> 100
    Serial.println("[SKEN] Start 170 -> 100 (Joints: PRIPREMA_PICKUP)");
}

void Manipulator::zapocniCekanjeQR() {
    qrCekanjeStart = millis();
    qrPocetnaKutBaze = trenutniKutovi[CH_BAZA];
    qrPrimljeno = false;
    trenutnoStanje = STANJE_QR_CEKANJE;
    Serial.println("[QR] Zapocinje cekanje QR-a (3s)...");
}

void Manipulator::primiQRSignal() {
    if (trenutnoStanje == STANJE_QR_CEKANJE ||
        trenutnoStanje == STANJE_QR_WIGGLE_LR ||
        trenutnoStanje == STANJE_QR_WIGGLE_RL) {
        qrPrimljeno = true;
        // Vrati bazu na originalnu poziciju
        int tmpKutovi[5];
        for (int i=0; i<5; i++) tmpKutovi[i] = (int)trenutniKutovi[i];
        tmpKutovi[CH_BAZA] = (int)qrPocetnaKutBaze;
        postaviPoziciju(tmpKutovi);
        trenutnoStanje = STANJE_MIRUJE;
        Serial.println("[QR] Signal primljen - wiggle zaustavljen.");
    }
}

int Manipulator::dohvatiPresetIdx() {

    return zadnjiPresetIdx;
}

String Manipulator::dohvatiNazivPozicije() {
    if (zadnjiPresetIdx == -1) return "Manual";
    if (trenutnoStanje == STANJE_PARKIRANJE) return "Parking";
    if (trenutnoStanje == STANJE_TEST_START_SAFE) return "Safe";
    
    // Ako smo u stanjima skeniranja, vrati poseban naziv da display zna pokazati TOF
    if (trenutnoStanje == STANJE_SKEN_LEVO || 
        trenutnoStanje == STANJE_SKEN_DESNO || 
        trenutnoStanje == STANJE_SKEN_NAMJESTI) {
        return "SKENIRANJE...";
    }
        
    if (trenutnoStanje >= STANJE_TEST_START_CEKANJE) return "Testna Sekvenca";

    if (zadnjiPresetIdx >= 0 && zadnjiPresetIdx < 16) {
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
    if (trenutnoVrijeme - zadnjeVrijeme < 10) {
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

        case STANJE_SAFE_PICKUP_K1:
            // Rame i zglob su stigli, sad LAKAT
            if (!kretanjeAct) {
                // Dodaj emo lakat i bazu i hvataljku (Baza i hvataljka obično idu na slično, ali za svaki slucaj stavljamo cijeli preset cilja)
                postaviPoziciju(pozicijaSafePickup);
                trenutnoStanje = STANJE_SAFE_PICKUP_K2;
            }
            break;

        case STANJE_SAFE_PICKUP_K2:
            if (!kretanjeAct) {
                trenutnoStanje = STANJE_MIRUJE; // Zavrsili
            }
            break;

        // --- TOF SKENIRANJE ZA PROVJERA_PICKUP ---
        case STANJE_SKEN_LEVO:
            // Uzorkuj TOF na svakom koraku
            if (tofFnPtr) {
                long tofVal = tofFnPtr();
                if (tofVal > 0 && tofVal < skenMinUdaljenost) {
                    skenMinUdaljenost = tofVal;
                    skenMinKut = trenutniKutovi[CH_BAZA];
                }
            }
            if (!kretanjeAct) {
                if (trenutniKutovi[CH_BAZA] > 100.1f) {
                    // Nastavi sweep prema 100
                    int ciljniKut = (int)trenutniKutovi[CH_BAZA] - 2; // Smanjuj po 2 stupnja
                    if (ciljniKut < 100) ciljniKut = 100;
                    postaviKut(CH_BAZA, (float)ciljniKut);
                } else {
                    // Stigli u 100°, sad namjesti bazu na optimalni kut
                    int tmpKutevi[5];
                    for (int i=0; i<5; i++) tmpKutevi[i] = (int)trenutniKutovi[i];
                    tmpKutevi[CH_BAZA] = (int)skenMinKut;
                    Serial.print("[SKEN] Optimalni kut: "); Serial.print(skenMinKut);
                    Serial.print(" ud: "); Serial.println(skenMinUdaljenost);
                    postaviPoziciju(tmpKutevi);
                    trenutnoStanje = STANJE_SKEN_NAMJESTI;
                }
            }
            break;

        case STANJE_SKEN_DESNO:
            // Više se ne koristi u ovom jednosmjernom sweepu
            trenutnoStanje = STANJE_SKEN_NAMJESTI;
            break;

        case STANJE_SKEN_NAMJESTI:
            if (!kretanjeAct) {
                trenutnoStanje = STANJE_MIRUJE; // Skeniranje gotovo
            }
            break;

        // --- QR CEKANJE I WIGGLE ---
        case STANJE_QR_CEKANJE:
            if (qrPrimljeno) {
                trenutnoStanje = STANJE_MIRUJE;
                break;
            }
            if (millis() - qrCekanjeStart > 3000) {
                // 3 sekunde prosle, pocni wiggle
                int tmpKutevi[5];
                for (int i=0; i<5; i++) tmpKutevi[i] = (int)trenutniKutovi[i];
                tmpKutevi[CH_BAZA] = (int)(qrPocetnaKutBaze - 2.0f);
                for (int i=0; i<5; i++) brzine[i] = 0.0;
                brzine[CH_BAZA] = 0.5; // Polako!
                postaviPoziciju(tmpKutevi);
                trenutnoStanje = STANJE_QR_WIGGLE_LR;
            }
            break;

        case STANJE_QR_WIGGLE_LR:
            if (qrPrimljeno) { trenutnoStanje = STANJE_MIRUJE; break; }
            if (!kretanjeAct) {
                // Stiglo u lijevu poziciju, idi desno
                int tmpKutevi[5];
                for (int i=0; i<5; i++) tmpKutevi[i] = (int)trenutniKutovi[i];
                tmpKutevi[CH_BAZA] = (int)(qrPocetnaKutBaze + 2.0f);
                postaviPoziciju(tmpKutevi);
                // Prekoraci osiguravaju spori korak
                brzine[CH_BAZA] = 0.5;
                trenutnoStanje = STANJE_QR_WIGGLE_RL;
            }
            break;

        case STANJE_QR_WIGGLE_RL:
            if (qrPrimljeno) { trenutnoStanje = STANJE_MIRUJE; break; }
            if (!kretanjeAct) {
                // Stiglo u desnu poziciju, idi lijevo opet
                int tmpKutevi[5];
                for (int i=0; i<5; i++) tmpKutevi[i] = (int)trenutniKutovi[i];
                tmpKutevi[CH_BAZA] = (int)(qrPocetnaKutBaze - 2.0f);
                postaviPoziciju(tmpKutevi);
                brzine[CH_BAZA] = 0.5;
                trenutnoStanje = STANJE_QR_WIGGLE_LR;
            }
            break;

        case STANJE_DROP_SEK_K1:
            if (!kretanjeAct) {
                // Svi motori stigli, sad otvori hvataljku
                postaviKut(CH_HVATALJKA, (float)odgodeniCiljevi[CH_HVATALJKA]);
                trenutnoStanje = STANJE_DROP_SEK_K2;
            }
            break;

        case STANJE_DROP_SEK_K2:
            if (!kretanjeAct) {
                trenutnoStanje = STANJE_MIRUJE;
            }
            break;

        default:
            break;
    }
}
