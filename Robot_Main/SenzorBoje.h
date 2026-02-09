/**
 * SenzorBoje.h
 * 
 * Modul za senzor boje APDS-9960.
 */

#ifndef SENZORBOJE_H
#define SENZORBOJE_H

#include <Arduino.h>

// ID-jevi boja
#define BOJA_NISTA 0
#define BOJA_CRVENA 1
#define BOJA_PLAVA 2
#define BOJA_ZUTA 3

/**
 * Inicijalizacija APDS-9960 senzora.
 */
void inicijalizirajSenzorBoje();

/**
 * Čita boju ispod senzora.
 * @return ID boje (0, 1, 2, 3)
 */
int citajBoju();

#endif // SENZORBOJE_H
