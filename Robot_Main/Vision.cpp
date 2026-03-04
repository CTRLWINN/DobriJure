/**
 * Vision.cpp
 * 
 * Implementacija komunikacije s kamerom.
 */

#include "Vision.h"
#include "HardwareMap.h"

String zadnjiQR = "";
String ulazniBuffer = "";
float visionError = 0.0; // -1.0 do 1.0
long visionUdaljenost = -1; // -1 znaci nema podatka
String visionIP = "0.0.0.0"; // IP od Nicle

void azurirajVision() {
    while (Serial3.available()) {
        char c = (char)Serial3.read();
        
        // ---> DEBUG MOGUĆNOST: Otkomentiraj ovo za live pregled u Arduino IDE Serijskom Monioru!
        Serial.print(c);
        
        if (c == '\n') {
            // Kraj poruke, parsiraj
            ulazniBuffer.trim(); // Ukloni whitespace
            
            // Format: "QR:Text"
            if (ulazniBuffer.startsWith("QR:")) {
                zadnjiQR = ulazniBuffer.substring(3);
                Serial.print("VISION: Novi QR -> ");
                Serial.println(zadnjiQR);
            }
            // Format: "LINE:error" (-1.0 to 1.0)
            else if (ulazniBuffer.startsWith("LINE:")) {
                visionError = ulazniBuffer.substring(5).toFloat();
                // Serial.println("VIS_ERR:" + String(visionError)); // Debug (spam)
            }
            // Format: "TOF:udaljenost" (mm)
            else if (ulazniBuffer.startsWith("TOF:")) {
                visionUdaljenost = ulazniBuffer.substring(4).toInt();
            }
            // Format: "IP:xxx.xxx.xxx.xxx"
            else if (ulazniBuffer.startsWith("IP:")) {
                visionIP = ulazniBuffer.substring(3);
            }
            
            ulazniBuffer = ""; // Reset buffera
        } else {
            ulazniBuffer += c;
        }
    }
}

String dohvatiZadnjiQR() {
    return zadnjiQR;
}

float dohvatiVisionError() {
    return visionError;
}

long dohvatiVisionUdaljenost() {
    return visionUdaljenost;
}

String dohvatiVisionIP() {
    return visionIP;
}

void obrisiZadnjiQR() {
    zadnjiQR = "";
}
