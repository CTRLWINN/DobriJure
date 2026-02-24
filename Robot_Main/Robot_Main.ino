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
    kalibrirajGyro();
    resetirajGyro();
    resetirajMag();
    
    // Postavi manipulator u parking poziciju
    ruka.parkiraj();

    // Smanji timeout za Serial2 da readString ne blokira dugo u petljama
    Serial2.setTimeout(5);
    
    Serial.println("Robot Ready. Waiting for commands...");
}

void loop() {
    ruka.azuriraj();
    azurirajIMU();

    if (Serial2.available()) {
        String msg = Serial2.readStringUntil('\n');
        msg.trim();
        
        if (msg.length() > 0) {
            Serial.print("CMD: ");
            Serial.println(msg);

            // --- MANUALNE KOMANDE (Tekstualni format) ---
            if (msg.startsWith("SERVO:")) {
                int commaIdx = msg.indexOf(',');
                if (commaIdx != -1) {
                    int ch = msg.substring(6, commaIdx).toInt();
                    float val = msg.substring(commaIdx + 1).toFloat();
                    
                    // Mapiranje Dashboard kanala 5 (Hvat) na robot kanal 4
                    if (ch == 5) ch = 4; 
                    
                    ruka.postaviKut(ch, val);
                    Serial2.println("{\"status\": \"OK\"}");
                }
                return;
            }
            
            if (msg.startsWith("LOAD_PRESET:")) {
                int idx = msg.substring(12).toInt();
                if (idx == 0) ruka.parkiraj();
                // Ostali preseti se mogu dodati ovdje
                Serial2.println("{\"status\": \"OK\"}");
                return;
            }

            // --- JSON KOMANDE ---
            DeserializationError error = deserializeJson(doc, msg);

            if (!error) {
                const char* cmd = doc["cmd"];

                if (strcmp(cmd, "straight") == 0) {
                    float val = doc["val"];
                    int spd = doc["spd"] | 0; // Default 0 -> BAZNA
                    voziRavno(val, spd);
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
                     int spd = doc["spd"] | 0;
                     okreni(val, spd);
                     Serial2.println("{\"status\": \"DONE\"}");
                }
                else if (strcmp(cmd, "pivot") == 0) {
                     float val = doc["val"];
                     int spd = doc["spd"] | 0;
                     skreni(val, spd);
                     Serial2.println("{\"status\": \"DONE\"}");
                }
                else if (strcmp(cmd, "arm") == 0) {
                     const char* val = doc["val"];
                     
                     if (strcmp(val, "PARKING") == 0 || strcmp(val, "Parkiraj") == 0 || strcmp(val, "HOME") == 0) {
                         ruka.parkiraj();
                         Serial2.println("{\"status\": \"OK\"}");
                     } else {
                         Serial.println("Preset not defined in new system yet.");
                         Serial2.println("{\"status\": \"ERR_NOT_IMPL\"}");
                     }
                }
                else if (strcmp(cmd, "read_sensor") == 0) {
                    const char* type = doc["type"];
                    if (strcmp(type, "induktivni") == 0) {
                        bool res = ocitajInduktivni();
                        Serial2.print("{\"status\": \"");
                        Serial2.print(res ? "1" : "0");
                        Serial2.println("\"}");
                    }
                }
                else if (strcmp(cmd, "set_motor") == 0) {
                    float val = doc["val"];
                    postaviKonfigKretanja(val); 
                    Serial2.println("{\"status\": \"OK\"}");
                }
                else if (strcmp(cmd, "set_move_cfg") == 0) {
                    float imp = doc["imp"] | 0.0;
                    int spd = doc["spd"] | 0;
                    float kp = doc["kp"] | -1.0;
                    int min_spd = doc["min"] | -1;
                    float impDeg = doc["deg"] | -1.0;
                    postaviParametre(imp, spd, kp, min_spd, impDeg);
                    Serial2.println("{\"status\": \"OK\"}");
                }
                else if (strcmp(cmd, "reset_enc") == 0) {
                    resetirajEnkodere();
                    Serial2.println("{\"status\": \"OK\"}");
                }
                else if (strcmp(cmd, "reset_imu") == 0) {
                    resetirajGyro();
                    resetirajMag();
                    Serial2.println("{\"status\": \"OK\"}");
                }
                else if (strcmp(cmd, "cal_imu") == 0) {
                    kalibrirajGyro();
                    resetirajGyro();
                    resetirajMag();
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
    
    posaljiTelemetriju();
}

void posaljiTelemetriju() {
    // --- TELEMETRIJA (svakih 200ms) ---
    static unsigned long zadnjaTelemetrija = 0;
    if (millis() - zadnjaTelemetrija > 200) {
        zadnjaTelemetrija = millis();
        
        long encL = dohvatiLijeviEnkoder();
        long encR = dohvatiDesniEnkoder();
        
        // Koristimo globalnu varijablu iz Kretanje.cpp za tocnu konverziju
        extern float IMPULSA_PO_CM; 
        float cm = (encL + encR) / 2.0 / IMPULSA_PO_CM; 
        
        int armIdx = 0; // ruka.dohvatiCiljaniPreset(); - TODO: Implementirati
        
        long usF = ocitajPrednjiUZ();
        long usB = ocitajStraznjiUZ();
        long usL = ocitajLijeviUZ();
        long usR = ocitajDesniUZ();
        
        bool ind = ocitajInduktivni();
        
        float yaw = dohvatiYaw(); 
        float gyro = dohvatiKutGyro();
        
        // Format: STATUS:cm,pL,pR,armIdx,usF,usB,usL,usR,ind,yaw,gyro
        Serial2.print("STATUS:");
        Serial2.print(cm, 1); Serial2.print(",");
        Serial2.print(encL); Serial2.print(",");
        Serial2.print(encR); Serial2.print(",");
        Serial2.print(armIdx); Serial2.print(",");
        Serial2.print(usF); Serial2.print(",");
        Serial2.print(usB); Serial2.print(",");
        Serial2.print(usL); Serial2.print(",");
        Serial2.print(usR); Serial2.print(",");
        Serial2.print(ind ? "1" : "0"); Serial2.print(",");
        Serial2.print(yaw, 1); Serial2.print(",");
        Serial2.println(gyro, 1);
    }
}
