#include "Vision.h"
#include "HardwareMap.h"

char trenutnaKameraMod = '0';
String zadnjiQR = "";
String ulazniBuffer = "";
float visionError = 0.0; // -1.0 do 1.0
long visionUdaljenost = -1; // -1 znaci nema podatka
String visionIP = "0.0.0.0"; // IP od Nicle

String odredisteMetal = "";
String odredistePlastika = "";
String odredisteSpuzva = "";

void azurirajVision() {
    while (Serial3.available()) {
        char c = (char)Serial3.read();
        
        // ---> DEBUG MOGUĆNOST: Otkomentiraj ovo za live pregled u Arduino IDE Serijskom Monioru!
        // Serial.print(c);
        
        if (c == '\n') {
            // Kraj poruke, parsiraj
            ulazniBuffer.trim(); // Ukloni whitespace
            
            // Format: "QR:Text"
            if (ulazniBuffer.startsWith("QR:")) {
                zadnjiQR = ulazniBuffer.substring(3); // npr: "M - D1; P - D2; S - D3"
                
                // Rastavite pomoću ';'
                int prviSep = zadnjiQR.indexOf(';');
                int drugiSep = zadnjiQR.indexOf(';', prviSep + 1);
                
                if (prviSep != -1 && drugiSep != -1) {
                    String prviDio = zadnjiQR.substring(0, prviSep);
                    String drugiDio = zadnjiQR.substring(prviSep + 1, drugiSep);
                    String treciDio = zadnjiQR.substring(drugiSep + 1);
                    
                    prviDio.trim();
                    drugiDio.trim();
                    treciDio.trim();
                    
                    String prepoznatiRedovi[3] = {prviDio, drugiDio, treciDio};
                    
                    // Mapirati ih prema prvom slovu i pospremiti varijable
                    for(int i = 0; i < 3; i++) {
                        String red = prepoznatiRedovi[i];
                        int crticaIndex = red.indexOf('-');
                        if(crticaIndex != -1) {
                            String vrijednostD = red.substring(crticaIndex + 1);
                            vrijednostD.trim();
                            
                            if(red.startsWith("M")) { odredisteMetal = vrijednostD; }
                            else if(red.startsWith("P")) { odredistePlastika = vrijednostD; }
                            else if(red.startsWith("S")) { odredisteSpuzva = vrijednostD; }
                        }
                    }
                    
                    // Ispiši na Serial onako kako je korisnik zatražio
                    Serial.println(prviDio);
                    Serial.println(drugiDio);
                    Serial.println(treciDio);
                } else {
                    // Fallback log
                    Serial.println(zadnjiQR);
                }
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

String dohvatiOdredisteMetal() {
    return odredisteMetal;
}

String dohvatiOdredistePlastika() {
    return odredistePlastika;
}

String dohvatiOdredisteSpuzva() {
    return odredisteSpuzva;
}

void postaviKameraMod(char mod) {
    trenutnaKameraMod = mod;
}

char dohvatiKameraMod() {
    return trenutnaKameraMod;
}
