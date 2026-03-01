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
 */
void azurirajDisplay(String qr, String pozicija, bool metal);

#endif // DISPLAY_H
