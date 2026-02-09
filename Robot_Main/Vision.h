/**
 * Vision.h
 * 
 * Modul za komunikaciju s Vision kamerom (Nicla Vision) preko Serial3.
 */

#ifndef VISION_H
#define VISION_H

#include <Arduino.h>

/**
 * Provjerava ima li novih podataka s kamere i ažurira interni buffer.
 * Pozivati u loop().
 */
void azurirajVision();

/**
 * Vraća zadnji pročitani QR kod.
 * @return String tekst QR koda.
 */
String dohvatiZadnjiQR();

/**
 * Vraća navigacijsku grešku s kamere (-1.0 do 1.0).
 * -1.0 = Linija skroz lijevo, 1.0 = Linija skroz desno.
 */
float dohvatiVisionError();

/**
 * Briše zadnji pročitani QR kod (npr. nakon obrade).
 */
void obrisiZadnjiQR();

#endif // VISION_H
