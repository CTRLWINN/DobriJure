/**
 * Enkoderi.h
 * 
 * Modul za čitanje kvadraturnih enkodera putem interrupta.
 */

#ifndef ENKODERI_H
#define ENKODERI_H

#include <Arduino.h>

/**
 * Inicijalizacija interrupta za enkodere. Pozvati iz setup().
 */
void inicijalizirajEnkodere();

/**
 * Dohvaća trenutnu poziciju lijevog enkodera.
 * @return broj pulseva
 */
long dohvatiLijeviEnkoder();

/**
 * Dohvaća trenutnu poziciju desnog enkodera.
 * @return broj pulseva
 */
long dohvatiDesniEnkoder();

/**
 * Resetira brojila enkodera na 0.
 */
void resetirajEnkodere();

#endif // ENKODERI_H
