/**
 * IMU.h
 * 
 * Modul za IMU senzor (LSM9DS1).
 * Fokus na dobivanje Yaw kuta (smjera).
 */

#ifndef IMU_H
#define IMU_H

#include <Arduino.h>

/**
 * Inicijalizira LSM9DS1 senzor.
 */
void inicijalizirajIMU();

/**
 * Ažurira očitanja senzora. Pozivati često u loopu.
 */
void azurirajIMU();

/**
 * Vraća trenutni Yaw kut (smjer) u stupnjevima (0 do 360).
 * Koristi magnetometar (kompas) za apsolutni smjer.
 * @return float kut 0.0 - 360.0
 */
float dohvatiYaw();

/**
 * Vraća ubrzanje po Z osi.
 * Korisno za detekciju udaraca/nagiba. (G-force)
 */
float dohvatiAccelZ();

#endif // IMU_H
