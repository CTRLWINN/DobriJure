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

void azurirajVision() {
    while (Serial3.available()) {
        char c = (char)Serial3.read();
        
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

void obrisiZadnjiQR() {
    zadnjiQR = "";
}
