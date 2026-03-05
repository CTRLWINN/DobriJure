#include "HardwareMap.h"
#include "Kretanje.h"
#include "Motori.h"
#include "Enkoderi.h"
#include "IMU.h"
#include "Manipulator.h"
#include "Vision.h"
#include "Display.h"
#include <ArduinoJson.h>

/*
 * Robot_Main.ino
 * 
 * Minimalisticki firmware za WSC 2026.
 * Sluša JSON komande preko Bluetooth-a (Serial2).
 * Hardware se inicijalizira vanjskim funkcijama.
 */

StaticJsonDocument<512> doc;
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
    
    // Inicijaliziraj display prije svega
    inicijalizirajDisplay();
    
    // --- STARTUP SEKVENCA ---
    // 1. Polako u parking i prikaz na ekranu (ceka 5 sekundi)
    prikaziVelikiTekst("PARKING");
    ruka.postaviPoziciju(pozicijaParking);
    unsigned long t = millis();
    while (millis() - t < 5000) {
        ruka.azuriraj();
        delay(20);
    }
    
    // 1.5 Cekanje gumba za pocetak misije (START Button)
    prikaziVelikiTekst("CEKANJE STARTA");
    while(!isStartPressed()) {
        ruka.azuriraj();
        // azurirajDisplay se ovdje ne zove jer tekst stoji fiksan
        delay(50);
    }

    
    // 2. Odlazak u SAFE poziciju i prikaz na ekranu (ceka 1.8 sekundi ukupno)
    prikaziVelikiTekst("SAFE");
    ruka.postaviPoziciju(pozicijaSafe);
    t = millis();
    while (millis() - t < 1800) {
        ruka.azuriraj();
        delay(20);
    }
    ruka.ucitajPreset(1); // Force robot to recognize it is in SAFE (1) at start
    // Nakon ovog normalni loop nastupa s ispisom šala.

    // Povećaj timeout za Serial2 da readString ne puca kod dužih JSON komada
    Serial2.setTimeout(50);
    
    Serial.println("Robot Ready. Waiting for commands...");
}

void loop() {
    ruka.azuriraj();
    azurirajIMU();
    azurirajVision();

    // Ažuriranje OLED ekrana (svakih 200ms da ne gušimo procesor)
    static unsigned long zadnjeAzuriranjeDisplaya = 0;
    if (millis() - zadnjeAzuriranjeDisplaya > 200) {
        zadnjeAzuriranjeDisplaya = millis();
        azurirajDisplay(
            dohvatiZadnjiQR(),
            ruka.dohvatiNazivPozicije(),
            ocitajInduktivni(),
            dohvatiVisionUdaljenost(),
            dohvatiKameraMod()
        );
    }

    if (Serial2.available()) {
        String msg = Serial2.readStringUntil('\n');
        msg.trim();
        
        if (msg.length() > 0) {
            Serial.print("CMD: ");
            Serial.println(msg);

            // --- MANUALNE KOMANDE (Tekstualni format) ---
            if (msg.startsWith("NICLA:")) {
                // Preusmjeravanje komande sa dashboarda na Niclin Serial3
                // msg je npr. "NICLA:u" ili "NICLA:0"
                if (msg.length() > 6) {
                    char commandForNicla = msg.charAt(6);
                    Serial3.print(commandForNicla);
                    postaviKameraMod(commandForNicla); // Snimamo mod da bi displej znao nacrtat format udaljenosti!
                    
                    Serial2.println("{\"status\": \"OK\"}");
                    Serial.print("Proslijedeno Nicli: ");
                    Serial.println(commandForNicla);
                }
                return;
            }
            
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
                ruka.ucitajPreset(idx);
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
                         int pronadjenIdx = -1;
                         for (int i = 0; i < 15; i++) {
                             if (strcmp(val, presetNames[i]) == 0) {
                                 pronadjenIdx = i;
                                 break;
                             }
                         }
                         
                         if (pronadjenIdx != -1) {
                             ruka.ucitajPreset(pronadjenIdx);
                             Serial2.println("{\"status\": \"OK\"}");
                         } else {
                             Serial.println("Preset not defined in new system yet.");
                             Serial2.println("{\"status\": \"ERR_NOT_IMPL\"}");
                         }
                     }
                }
                else if (strcmp(cmd, "arm_pos") == 0) {
                     JsonArray arr = doc["angles"];
                     int kuteviInt[5];
                     for(int i=0; i<5; i++) {
                         // as<int>() je sigurniji način od varijantnog OR operatora u ovoj petlji
                         kuteviInt[i] = arr[i].as<int>();
                     }
                     int p_idx = doc["preset_idx"] | -1;
                     ruka.postaviSve(kuteviInt, p_idx);
                     Serial2.println("{\"status\": \"OK\"}");
                }
                else if (strcmp(cmd, "nicla") == 0) {
                     // Auto-Misija salje "nicla" json. Prosljedujemo prvi (jedini) znak parametra mod prema Serial3
                     const char* modStr = doc["mod"];
                     if (modStr && strlen(modStr) > 0) {
                         char nicla_cmd = modStr[0];
                         Serial3.print(nicla_cmd);
                         postaviKameraMod(nicla_cmd);
                         Serial2.println("{\"status\": \"OK\"}");
                         Serial.print("Auto-Misija prebacuje kameru u mod: ");
                         Serial.println(nicla_cmd);
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
                else if (strcmp(cmd, "wait_start") == 0) {
                    // Cekaj pritisak fizickog START gumba (pin 23)
                    // U meduvremenu nastavljamo slati telemetriju da BLE ne ispada
                    Serial.println("Cekam START gumb...");
                    while (!isStartPressed()) {
                        posaljiTelemetriju();
                        ruka.azuriraj();
                        delay(50);
                    }
                    Serial2.println("{\"status\": \"DONE\"}");
                    Serial.println("START gumb pritisnut!");
                }
                else {
                    Serial.println("Unknown command.");
                }
            } else {
                // FALLBACK: Ako nije JSON, možda je stara raw komanda (npr. ARM:1 ili NICLA:q)
                String raw = msg;
                if (raw.startsWith("ARM:")) {
                    int idx = raw.substring(4).toInt();
                    ruka.ucitajPreset(idx);
                }
                else if (raw.startsWith("NICLA:")) {
                    char m = raw.charAt(6);
                    Serial3.print(m);
                    postaviKameraMod(m);
                }
                else if (raw.startsWith("SAVE_PRESET:")) {
                    // Ignoriramo po želji korisnika (ne sprema u EEPROM)
                }
                else if (raw.startsWith("LOAD_PRESET:")) {
                    int idx = raw.substring(12).toInt();
                    ruka.ucitajPreset(idx);
                }
                else if (raw.startsWith("MAN:")) {
                    // Manual kretanje: MAN:FWD,200
                    String sub = raw.substring(4);
                    int comma = sub.indexOf(',');
                    if (comma != -1) {
                        String dir = sub.substring(0, comma);
                        int spd = sub.substring(comma+1).toInt();
                        if (dir == "FWD") vozi(spd, spd);
                        else if (dir == "BCK") vozi(-spd, -spd);
                        else if (dir == "LFT") vozi(-spd, spd);
                        else if (dir == "RGT") vozi(spd, -spd);
                    }
                }
                else if (raw == "STOP") {
                    stani();
                }
                
                Serial.print("Raw komanda obrađena: ");
                Serial.println(raw);
            }
        }
    }
    
    posaljiTelemetriju();
}

void posaljiTelemetriju() {
    // --- TELEMETRIJA (svakih 100ms) ---
    static unsigned long zadnjaTelemetrija = 0;
    if (millis() - zadnjaTelemetrija > 100) {
        zadnjaTelemetrija = millis();
        
        long encL = dohvatiLijeviEnkoder();
        long encR = dohvatiDesniEnkoder();
        
        // Koristimo globalnu varijablu iz Kretanje.cpp za tocnu konverziju
        extern float IMPULSA_PO_CM; 
        float cm = (encL + encR) / 2.0 / IMPULSA_PO_CM; 
        
        int armIdx = ruka.dohvatiPresetIdx(); 
        
        long usF = ocitajPrednjiUZ();
        long usB = ocitajStraznjiUZ();
        long usL = ocitajLijeviUZ();
        long usR = ocitajDesniUZ();
        
        bool ind = ocitajInduktivni();
        
        float yaw = dohvatiYaw(); 
        float gyro = dohvatiKutGyro();
        float gz = dohvatiGyroZ();
        long tof = dohvatiVisionUdaljenost();
        String ip = dohvatiVisionIP();
        
        String destM = dohvatiOdredisteMetal(); if (destM == "") destM = "-";
        String destP = dohvatiOdredistePlastika(); if (destP == "") destP = "-";
        String destS = dohvatiOdredisteSpuzva(); if (destS == "") destS = "-";
        
        // Format: STATUS:cm,pL,pR,armIdx,usF,usB,usL,usR,ind,yaw,gyro,gz,tof,ip,destM,destP,destS
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
        Serial2.print(gyro, 1); Serial2.print(",");
        Serial2.print(gz, 1); Serial2.print(",");
        Serial2.print(tof); Serial2.print(",");
        Serial2.print(ip); Serial2.print(",");
        Serial2.print(destM); Serial2.print(",");
        Serial2.print(destP); Serial2.print(",");
        Serial2.println(destS);
    }
}
