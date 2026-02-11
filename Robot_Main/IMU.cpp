/**
 * IMU.cpp
 * 
 * Implementacija IMU funkcionalnosti.
 */

#include "IMU.h"
#include <Wire.h>
#include <SparkFunLSM9DS1.h> 

LSM9DS1 imu;

// Varijabla za spremanje kalibracijskih podataka kompasa ako postoje
float magMinX = 0, magMaxX = 0;
float magMinY = 0, magMaxY = 0;
bool kalibriran = false;

// Gyro Bias (Drift Correction)
float gyroBiasZ = 0;
// Mag Offset (Zeroing)
float magHeadingOffset = 0;

// Gyro integracija
float gyroAngleZ = 0;
unsigned long lastUpdateMicros = 0;

void inicijalizirajIMU() {
    Serial.println("IMU Inicijalizacija...");
    
    // Inicijalizacija na zadanim I2C adresama (0x1E za Mag, 0x6B za Accel/Gyro)
    imu.settings.device.commInterface = IMU_MODE_I2C;
    imu.settings.device.mAddress = 0x1E;
    imu.settings.device.agAddress = 0x6B;

    if (!imu.begin()) {
        Serial.println("GRESKA: IMU nije pronaden!");
    } else {
        Serial.println("IMU spojen.");
        // Početno čitanje za postavljanje tajmera
        imu.readGyro();
        lastUpdateMicros = micros();
        
        // Odmah napravi brzu kalibraciju pri paljenju
        kalibrirajGyro();
    }
}

void kalibrirajGyro() {
    Serial.println("Kalibracija Ziroskopa (mirujte)...");
    float sumZ = 0;
    int sampleCount = 500;
    
    for(int i=0; i<sampleCount; i++) {
        if (imu.gyroAvailable()) {
            imu.readGyro();
            sumZ += imu.calcGyro(imu.gz);
        }
        delay(3);
    }
    
    gyroBiasZ = sumZ / sampleCount;
    Serial.print("Gyro Bias Z: ");
    Serial.println(gyroBiasZ);
}

void postaviKalibraciju(float xMin, float xMax, float yMin, float yMax) {
    magMinX = xMin;
    magMaxX = xMax;
    magMinY = yMin;
    magMaxY = yMax;
    kalibriran = true;
}

void resetirajGyro() {
    gyroAngleZ = 0;
    lastUpdateMicros = micros();
}

void azurirajIMU() {
    // Čitanje magnetometra i akcelerometra
    if (imu.magAvailable()) imu.readMag();
    if (imu.accelAvailable()) imu.readAccel();
    
    // Gyro Integracija
    if (imu.gyroAvailable()) {
        imu.readGyro();
        
        unsigned long now = micros();
        float dt = (now - lastUpdateMicros) / 1000000.0;
        lastUpdateMicros = now;

        // Očitaj Z os (stupnjevi po sekundi)
        float gz = imu.calcGyro(imu.gz);
        
        // Oduzmi bias
        gz -= gyroBiasZ;
        
        // Deadzone filter (šum)
        if (abs(gz) < 1.0) gz = 0; 
        
        // Integracija (zbrajanje kuta)
        // Pazi na predznak: ovisi o montaži čipa. 
        // Obično CCW (lijevo) je pozitivno.
        gyroAngleZ += gz * dt; 
    }
}

float dohvatiKutGyro() {
    return gyroAngleZ;
}

float dohvatiYaw() {
    // Jednostavan izračun kuta iz magnetometra (kompas)
    float mx = imu.mx;
    float my = imu.my;
    
    if (kalibriran) {
        float xOffset = (magMinX + magMaxX) / 2;
        float yOffset = (magMinY + magMaxY) / 2;
        mx -= xOffset;
        my -= yOffset;
    }
    
    float heading = atan2(my, mx);
    
    // Korekcija intervala [0, 2*PI]
    if (heading < 0) heading += 2 * PI;
    if (heading > 2 * PI) heading -= 2 * PI;

    float deg = heading * 180.0 / PI;
    
    // Primjeni offset (nulovanje)
    deg -= magHeadingOffset;
    
    // Normaliziraj na 0-360
    if (deg < 0) deg += 360.0;
    if (deg >= 360.0) deg -= 360.0;

    return deg;
}

void resetirajMag() {
    // Spremi trenutni raw heading kao offset
    float mx = imu.mx;
    float my = imu.my;
    
    if (kalibriran) {
        float xOffset = (magMinX + magMaxX) / 2;
        float yOffset = (magMinY + magMaxY) / 2;
        mx -= xOffset;
        my -= yOffset;
    }
    
    float rawHeading = atan2(my, mx);
    if (rawHeading < 0) rawHeading += 2 * PI;
    if (rawHeading > 2 * PI) rawHeading -= 2 * PI;
    
    magHeadingOffset = rawHeading * 180.0 / PI;
}

float dohvatiAccelZ() {
    return imu.calcAccel(imu.az);
}
