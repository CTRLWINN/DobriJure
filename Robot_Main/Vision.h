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
 * Vraća udaljenost izmjerenu TOF senzorom u milimetrima.
 */
long dohvatiVisionUdaljenost();

/**
 * Zapisuje trenutno poznati mod kamere s kojim Robot radi.
 */
void postaviKameraMod(char mod);

/**
 * Dohvaća zadnje poslani mod prema kameri.
 */
char dohvatiKameraMod();

/**
 * Vraća IP adresu Nicle na wifiju
 */
String dohvatiVisionIP();

/**
 * Briše zadnji pročitani QR kod (npr. nakon obrade).
 */
void obrisiZadnjiQR();

/**
 * Vraća lokaciju za spremnik pojedinog materijala (npr. "D1", "D2")
 */
String dohvatiOdredisteMetal();
String dohvatiOdredistePlastika();
String dohvatiOdredisteSpuzva();

#endif // VISION_H
