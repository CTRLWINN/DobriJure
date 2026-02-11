#include "HardwareMap.h"
#include "Kretanje.h"
#include "Motori.h"
#include "Enkoderi.h"
#include "IMU.h"
#include "Manipulator.h"
#include <ArduinoJson.h>

/*
 * Robot_Main.ino
 * 
 * Minimalisticki firmware za WSC 2026.
 * Sluša JSON komande preko Bluetooth-a (Serial2).
 * Hardware se inicijalizira vanjskim funkcijama.
 */

StaticJsonDocument<200> doc;
Manipulator ruka;

void setup() {
    // 1. Inicijalizacija Hardware pinova i Serial portova
    inicijalizirajHardware(); // Postavlja pinove, Serial, Serial2, I2C
    
    // 2. Dodatne inicijalizacije modula
    inicijalizirajEnkodere();
    inicijalizirajIMU();
    
    // Osiguraj da robot stoji
    stani();
    
    Serial.println("Robot Ready. Waiting for commands...");
}

void loop() {
    ruka.azuriraj();

    if (Serial2.available()) {
        String msg = Serial2.readStringUntil('\n');
        msg.trim();
        
        if (msg.length() > 0) {
            Serial.print("CMD: ");
            Serial.println(msg);

            DeserializationError error = deserializeJson(doc, msg);

            if (!error) {
                const char* cmd = doc["cmd"];

                if (strcmp(cmd, "straight") == 0) {
                    float val = doc["val"];
                    voziRavno(val);
                    Serial2.println("{\"status\": \"DONE\"}");
                }
                else if (strcmp(cmd, "move_dual") == 0) {
                    int l = doc["l"];
                    int r = doc["r"];
                    float dist = doc["dist"] | 0.0; // Default to 0 if missing
                    vozi(l, r, dist);
                    Serial2.println("{\"status\": \"OK\"}");
                }
                else if (strcmp(cmd, "stop") == 0) {
                    stani();
                    Serial2.println("{\"status\": \"STOPPED\"}");
                }
                else if (strcmp(cmd, "turn") == 0) {
                     float val = doc["val"];
                     okreni(val);
                     Serial2.println("{\"status\": \"DONE\"}");
                }
                else if (strcmp(cmd, "pivot") == 0) {
                     float val = doc["val"];
                     skreni(val);
                     Serial2.println("{\"status\": \"DONE\"}");
                }
                else if (strcmp(cmd, "arm") == 0) {
                     const char* val = doc["val"];
                     int idx = -1;
                     // Trazi preset po imenu
                     for(int i=0; i<15; i++) {
                         if (presetNames[i] && strcmp(presetNames[i], val) == 0) {
                             idx = i;
                             break;
                         }
                     }
                     
                     if (idx != -1) {
                         ruka.primjeniPreset(idx);
                         Serial2.println("{\"status\": \"OK\"}");
                     } else {
                         // Fallback: Mozda je poslan broj kao string?
                         idx = atoi(val);
                         if (idx >= 0 && idx < 15) {
                             ruka.primjeniPreset(idx);
                             Serial2.println("{\"status\": \"OK\"}");
                         } else {
                             Serial.println("Unknown Preset");
                             Serial2.println("{\"status\": \"ERR\"}");
                         }
                     }
                }
                else if (strcmp(cmd, "set_motor") == 0) {
                    float val = doc["val"];
                    postaviKonfigKretanja(val);
                    Serial2.println("{\"status\": \"OK\"}");
                }
                else {
                    Serial.println("Unknown command.");
                }
            } else {
                Serial.print("JSON Error: ");
                Serial.println(error.c_str());
            }
        }
    }
}
