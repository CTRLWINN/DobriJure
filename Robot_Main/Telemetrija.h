/**
 * Telemetrija.h
 * 
 * Modul za slanje telemetrije na PC Dashboard preko Bluetootha (Serial2).
 * Protokol: JSON
 */

#ifndef TELEMETRIJA_H
#define TELEMETRIJA_H

#include <Arduino.h>

/**
 * Šalje paket telemetrije u JSON formatu.
 * Format: {"enc_l": int, "enc_r": int, "spd": float, "head": float}
 * 
 * @param encL Broj pulseva lijevog enkodera
 * @param encR Broj pulseva desnog enkodera
 * @param spd Trenutna brzina (m/s)
 * @param head Orijentacija (Yaw) u stupnjevima
 */
void posaljiTelemetrijuJSON(long encL, long encR, float spd, float head);

#endif // TELEMETRIJA_H
