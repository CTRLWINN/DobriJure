#ifndef KRETANJE_H
#define KRETANJE_H

#include <Arduino.h>

/**
 * Pravocrtno kretanje (Encoder Only).
 * @param cm Udaljenost u cm.
 * @param speed Brzina (0 = koristi BAZNA_BRZINA).
 */
void voziRavno(float cm, int speed = 0);

/**
 * Pokreće motore zadanom brzinom.
 * Ne blokira. Ne staje sama.
 * @param lijeviMotor Brzina lijevog (-255 do 255)
 * @param desniMotor Brzina desnog (-255 do 255)
 */
void vozi(int lijeviMotor, int desniMotor);

/**
 * Okret na mjestu.
 * @param kut Kut u stupnjevima.
 * @param speed Brzina (0 = koristi BAZNA_BRZINA).
 */
void okreni(float kut, int speed = 0);

/**
 * Pivot turn.
 * @param kut Kut u stupnjevima.
 * @param speed Brzina (0 = koristi BAZNA_BRZINA).
 */
void skreni(float kut, int speed = 0);

/**
 * Vozi zadanim brzinama motora do predene udaljenosti.
 * BLOKIRAJUCA FUNKCIJA.
 * @param lijeviMotor Brzina lijevog (-255 do 255)
 * @param desniMotor Brzina desnog (-255 do 255)
 * @param cm Udaljenost u cm (prosjek oba kotaca). Ako je 0, vozi beskonacno (ne blokira).
 */
void vozi(int lijeviMotor, int desniMotor, float cm);

void postaviParametre(float imp, int brzina, float kp, int minBrzina, float impDeg);

void postaviKonfigKretanja(float impulsaPoCm);
// Provjerava Serial2 za "stop" naredbu. Vraca true ako je stopiran.
bool provjeriHitniStop();

void zaustaviKretanje();
void stani();
// Helper za slanje telemetrije iz blokirajucih petlji
extern void posaljiTelemetriju();

#endif
