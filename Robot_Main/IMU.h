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
 * Postavlja kalibracijske parametre (hard-coded ili iz EEPROM-a).
 * @param xMin Minimalna vrijednost X osi magnetometra.
 * @param xMax Maksimalna vrijednost X osi magnetometra.
 * @param yMin Minimalna vrijednost Y osi magnetometra.
 * @param yMax Maksimalna vrijednost Y osi magnetometra.
 */
void postaviKalibraciju(float xMin, float xMax, float yMin, float yMax);

/**
 * Kalibrira žiroskop (računa bias/drift) dok robot miruje.
 * Traje cca 1-2 sekunde.
 */
void kalibrirajGyro();

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
 * Vraća integrirani kut iz žiroskopa (Z-os).
 * 0 je tamo gdje je robot bio kod zadnjeg resetirajGyro().
 * Pozitivno = Lijevo (CCW), Negativno = Desno (CW).
 */
float dohvatiKutGyro();

/**
 * Resetira integrirani kut žiroskopa na 0.
 */
void resetirajGyro();

/**
 * Postavlja trenutni očitani heading magnetometra kao "0 stupnjeva".
 * Korisno za resetiranje smjera bez fizičkog pomicanja robota.
 */
void resetirajMag();

/**
 * Vraća ubrzanje po Z osi.
 * Korisno za detekciju udaraca/nagiba. (G-force)
 */
float dohvatiAccelZ();

#endif // IMU_H
