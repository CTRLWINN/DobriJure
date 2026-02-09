/**
 * Enkoderi.cpp
 * 
 * Implementacija interrupta za enkodere.
 */

#include "Enkoderi.h"
#include "HardwareMap.h"

// Varijable moraju biti volatile jer se mijenjaju unutar ISR-a (Interrupt Service Routine)
volatile long pozicijaLijevi = 0;
volatile long pozicijaDesni = 0;

// ISR funkcije za lijevi enkoder
void isrLijevi() {
    // Čitamo stanje drugog kanala da odredimo smjer
    if (digitalRead(PIN_ENC_L_A) == digitalRead(PIN_ENC_L_B)) {
        pozicijaLijevi++;
    } else {
        pozicijaLijevi--;
    }
}

// ISR funkcije za desni enkoder
void isrDesni() {
    if (digitalRead(PIN_ENC_R_A) == digitalRead(PIN_ENC_R_B)) {
        pozicijaDesni--; // OBRNUTO ZBOG MONTAŽE
    } else {
        pozicijaDesni++; // OBRNUTO ZBOG MONTAŽE
    }
}

void inicijalizirajEnkodere() {
    // Pretpostavljamo da su pinovi postavljeni na INPUT_PULLUP u HardwareMap.cpp
    // Okidamo na PROMJENU stanja (CHANGE) za veću rezoluciju ili RISING/FALLING.
    // Za jednostavnost i osnovnu preciznost koristimo RISING na kanalu A. 
    // Ako treba veća rezolucija (X2 ili X4), dodati interrupt i na kanal B ili CHANGE.
    
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_L_A), isrLijevi, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_R_A), isrDesni, RISING);
}

long dohvatiLijeviEnkoder() {
    // Atomsko čitanje za 32-bitne varijable na 8-bitnom MCU
    long temp;
    noInterrupts();
    temp = pozicijaLijevi;
    interrupts();
    return temp;
}

long dohvatiDesniEnkoder() {
    long temp;
    noInterrupts();
    temp = pozicijaDesni;
    interrupts();
    return temp;
}

void resetirajEnkodere() {
    noInterrupts();
    pozicijaLijevi = 0;
    pozicijaDesni = 0;
    interrupts();
}
