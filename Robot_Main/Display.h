/**
 * Display.h
 * 
 * Modul za upravljanje OLED ekranom SSD1306.
 * Prikazuje telemetriju i ironične komentare.
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

/**
 * Inicijalizira OLED ekran.
 */
void inicijalizirajDisplay();

/**
 * Ažurira sadržaj ekrana.
 * @param qr Zadnji pročitani QR kod
 * @param pozicija Naziv trenutne pozicije manipulatora
 * @param metal True ako je detektiran metal
 * @param tof Udaljenost izmjerena TOF senzorom
 */
void azurirajDisplay(String qr, String pozicija, bool metal, long tof, char kameraMod = '0');

/**
 * Prikazuje veliki centrirani tekst preko cijelog ekrana.
 */
void prikaziVelikiTekst(String tekst);

/**
 * Prikazuje IP adresu kamere dok se ceka pocetak misije.
 * @param ip zadnja pročitana IP adresa Nicla Vision kamere
 */
void azurirajDisplayCekanjeStarta(String ip);

#endif // DISPLAY_H
