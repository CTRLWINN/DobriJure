/**
 * Telemetrija.cpp
 * 
 * Implementacija slanja podataka u JSON formatu.
 */

#include "Telemetrija.h"
#include "HardwareMap.h"

unsigned long zadnjeSlanje = 0;
const unsigned long INTERVAL_SLANJA = 100; // 10Hz

void posaljiTelemetrijuJSON(long encL, long encR, float spd, float head) {
    if (millis() - zadnjeSlanje >= INTERVAL_SLANJA) {
        // Format: {"enc_l": 123, "enc_r": 123, "spd": 0.5, "head": 12.5}
        
        Serial2.print("{\"enc_l\": ");
        Serial2.print(encL);
        Serial2.print(", \"enc_r\": ");
        Serial2.print(encR);
        Serial2.print(", \"spd\": ");
        Serial2.print(spd, 2);
        Serial2.print(", \"head\": ");
        Serial2.print(head, 2);
        Serial2.println("}");
        
        zadnjeSlanje = millis();
    }
}
