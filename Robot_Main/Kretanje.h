#ifndef KRETANJE_H
#define KRETANJE_H

#include <Arduino.h>

/**
 * Pravocrtno kretanje (Encoder Only).
 * @param cm Udaljenost u cm.
 */
void voziRavno(float cm);

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
 */
void okreni(float kut);

/**
 * Pivot turn.
 * @param kut Kut u stupnjevima.
 */
void skreni(float kut);

/**
 * Vozi zadanim brzinama motora do predene udaljenosti.
 * BLOKIRAJUCA FUNKCIJA.
 * @param lijeviMotor Brzina lijevog (-255 do 255)
 * @param desniMotor Brzina desnog (-255 do 255)
 * @param cm Udaljenost u cm (prosjek oba kotaca). Ako je 0, vozi beskonacno (ne blokira).
 */
void vozi(int lijeviMotor, int desniMotor, float cm);

void postaviKonfigKretanja(float impulsaPoCm);
void zaustaviKretanje();
void stani();

#endif
