/**
 * IMU.cpp
 * 
 * Implementacija IMU funkcionalnosti.
 */

#include "IMU.h"
#include <Wire.h>
#include <SparkFunLSM9DS1.h> // Pretpostavljena biblioteka

LSM9DS1 imu;

// Varijabla za spremanje kalibracijskih podataka kompasa ako postoje
// float magOffset[3] = {0, 0, 0}; 

void inicijalizirajIMU() {
    Serial.println("IMU Inicijalizacija...");
    
    // Inicijalizacija na zadanim I2C adresama (0x1E za Mag, 0x6B za Accel/Gyro)
    // HardwareMap.h kaže 0x1E.
    imu.settings.device.commInterface = IMU_MODE_I2C;
    imu.settings.device.mAddress = 0x1E;
    imu.settings.device.agAddress = 0x6B;

    if (!imu.begin()) {
        Serial.println("GRESKA: IMU nije pronaden!");
        // Ovdje bi moglo ići blokiranje ili signalizacija greške
    } else {
        Serial.println("IMU spojen.");
    }
}

void azurirajIMU() {
    // Čitanje magnetometra i akcelerometra
    if (imu.magAvailable()) {
        imu.readMag();
    }
    if (imu.accelAvailable()) {
        imu.readAccel();
    }
}

float dohvatiYaw() {
    // Jednostavan izračun kuta iz magnetometra (kompas)
    // Pretpostavlja se da je senzor montiran ravno
    
    float mx = imu.mx;
    float my = imu.my;
    
    // Izračun kuta u radijanima
    float heading = atan2(my, mx);
    
    // Deklinacija (korekcija za magnetski sjever vs pravi sjever)
    // Za Hrvatsku je oko 4-5 stupnjeva, ovdje zanemarujemo ili se može dodati.
    // float declinationAngle = 0.08; 
    // heading += declinationAngle;

    // Korekcija intervala [0, 2*PI]
    if (heading < 0) {
        heading += 2 * PI;
    }
    if (heading > 2 * PI) {
        heading -= 2 * PI;
    }

    // Konverzija u stupnjeve
    float headingDegrees = heading * 180.0 / PI;

    return headingDegrees;
}

float dohvatiAccelZ() {
    return imu.calcAccel(imu.az);
}
