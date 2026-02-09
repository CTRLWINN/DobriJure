/**
 * Ultrazvuk.h
 * 
 * Modul za mjerenje udaljenosti ultrazvučnim senzorima HC-SR04.
 */

#ifndef ULTRAZVUK_H
#define ULTRAZVUK_H

#include <Arduino.h>

// Imenovani smjerovi za lakše korištenje
#define SMJER_NAPRIJED 0
#define SMJER_NAZAD    1
#define SMJER_LIJEVO   2
#define SMJER_DESNO    3
#define SMJER_GRIPPER  4

/**
 * Mjeri udaljenost u odabranom smjeru.
 * @param smjer Konstanta SMJER_...
 * @return Udaljenost u centimetrima. Vraća -1 ako je izvan dometa ili greška.
 */
long udaljenost(int smjer);

#endif // ULTRAZVUK_H
