#ifndef KRETANJE_H
#define KRETANJE_H

#include <Arduino.h>

// Stanja kretanja
enum StanjeKretanja {
    KRETANJE_IDLE,
    KRETANJE_RAVNO,
    KRETANJE_ROTACIJA,
    KRETANJE_LINIJA,
    KRETANJE_PIVOT,
    KRETANJE_DUAL
};

void zapocniVoznju(float cm);
void zapocniRotaciju(float stupnjevi);
void straightDrive(float cm); // Wrapper za JSON
void pivotTurn(float kut);
void differentialDrive(int l, int r, float dist);
void pokreniPracenjeLinije();
void zaustaviKretanje();
float dohvatiPredjeniPutCm();
bool jeUPokretu();
void azurirajKretanje();
void postaviKonfigKretanja(float impulsaPoCm);

// Nove funkcije
void voziRavno(float cm);
void resetirajEnkodereCmd();

#endif
