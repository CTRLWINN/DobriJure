#include "Manipulator.h"

Manipulator::Manipulator() {
    trenutnoStanje = STANJE_MIRUJE;
    tipSekvence = "";
    zadnjeVrijeme = 0;

    // Inicijalizacija na parking poziciju
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

    return map((long)kut, 0, maxStupnjeva, SERVO_MIN, SERVO_MAX);
}

void Manipulator::postaviPoziciju(const int poz[5]) {
    // Izračunaj korak za svaki motor da bi stigli istovremeno (150 koraka kao u testu)
    for (int i = 0; i < 5; i++) {
        ciljaniKutovi[i] = poz[i];
        brzine[i] = (ciljaniKutovi[i] - trenutniKutovi[i]) / 150.0;
    }
}

void Manipulator::postaviKut(int kanal, float stupnjevi) {
    if (kanal < 0 || kanal >= 5) return;
    ciljaniKutovi[kanal] = stupnjevi;
    // Za pojedinačni kut koristimo default brzinu
    brzine[kanal] = (stupnjevi > trenutniKutovi[kanal]) ? KORAK_MK : -KORAK_MK;
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
    postaviPoziciju(pozicijaParking);
    trenutnoStanje = STANJE_PARKIRANJE;
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
        
        if (abs(razlika) > abs(brzine[i]) && abs(razlika) > 0.1) {
            trenutniKutovi[i] += brzine[i];
            kretanjeAct = true;
        } else {
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
            
        default:
            break;
    }
}
