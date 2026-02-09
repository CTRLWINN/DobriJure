/*
  IMU_Calibration.ino
  
  Skica za kalibraciju magnetometra na LSM9DS1.
  Vrti robota u krug i prati Min/Max vrijednosti.
  
  Spoj: 
  SDA -> 20 (Mega)
  SCL -> 21 (Mega)
*/

#include <Wire.h>
#include <SparkFunLSM9DS1.h>

LSM9DS1 imu;

float magMinX = 10000;
float magMaxX = -10000;
float magMinY = 10000;
float magMaxY = -10000;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  imu.settings.device.commInterface = IMU_MODE_I2C;
  imu.settings.device.mAddress = 0x1E;
  imu.settings.device.agAddress = 0x6B;

  if (!imu.begin()) {
    Serial.println("Greska: IMU nije pronaden. Provjeri spoj!");
    while (1);
  }

  Serial.println("IMU Kalibracija zapoceta.");
  Serial.println("Rotiraj robota polako za punih 360 stupnjeva (i vise puta).");
  Serial.println("Saljem podatke: MinX, MaxX, MinY, MaxY");
  delay(2000);
}

void loop() {
  if (imu.magAvailable()) {
    imu.readMag();
    
    float x = imu.mx;
    float y = imu.my;
    
    // Ažuriraj Min/Max
    if (x < magMinX) magMinX = x;
    if (x > magMaxX) magMaxX = x;
    
    if (y < magMinY) magMinY = y;
    if (y > magMaxY) magMaxY = y;
    
    // Ispis svakih 100ms
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 100) {
        Serial.print("X: "); Serial.print(x);
        Serial.print(" Y: "); Serial.print(y);
        
        Serial.print(" | CALIB -> ");
        Serial.print("MinX: "); Serial.print(magMinX);
        Serial.print(" MaxX: "); Serial.print(magMaxX);
        Serial.print(" MinY: "); Serial.print(magMinY);
        Serial.print(" MaxY: "); Serial.println(magMaxY);
        
        lastPrint = millis();
    }
  }
}
