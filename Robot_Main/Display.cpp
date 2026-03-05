/**
 * Display.cpp
 * 
 * Implementacija prikaza na OLED ekranu.
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Fonts/FreeSans9pt7b.h>
#include "Display.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 
#define SCREEN_ADDRESS 0x3C 

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pametna Word-Wrap funkcija za ljepsi izlazni prikaz s defaultnim fontom
void prikaziOmotanTekst(const char* text, int x, int y, int maxW) {
    display.setFont(); // Sigurnosti radi, vracamo standardni sitniji font (5x7 px po slovu)
    display.setTextSize(1);
    
    String str = String(text);
    int start = 0;
    int curY = y;
    
    while(start < str.length()) {
        int end = start;
        int lastSpace = -1;
        while(end < str.length()) {
            if(str.charAt(end) == ' ') lastSpace = end;
            String substr = str.substring(start, end + 1);
            int16_t x1, y1; uint16_t w, h;
            display.getTextBounds(substr, 0, 0, &x1, &y1, &w, &h);
            if(w > maxW) {
                if(lastSpace > start) {
                    end = lastSpace;
                } else {
                    end--; // Riječ je predugačka, lomi prisilno
                }
                break;
            }
            if(end == str.length() - 1) break; // Došli smo do kraja stringa
            end++;
        }
        
        display.setCursor(x, curY);
        display.print(str.substring(start, end + (end == str.length() - 1 ? 1 : 0)));
        curY += 10; // Standardna visina s proredom na sitnom fontu je dovoljna oko 10px
        
        start = end + 1;
    }
}

// Marvin-style ironični komentari
const char* marvinQuotesGeneral[] = {
    "Opet telemetrija? Kako uzbudljivo. (Nije)",
    "Zivot? Ne pricaj mi o zivotu.",
    "Depresivan sam. Cak je i ovaj ekran crnobijel.",
    "Cekam da mi baterija iscuri. Vrhunac dana.",
    "Moja lijeva dioda me boli više nego tvoja pitanja.",
    "Mogao bih izracunati put do ruba galaksije, ali ne...",
    "Struja tece, ali volja za zivotom ne.",
    "Jos jedan besmisleni loop u beskraju koda.",
    "Nadam se da ce uskoro nestati struje."
};
const int numGeneral = 9;

const char* marvinQuotesArm[] = {
    "Evo, pomaknuo sam ruku. Jesi li sad sretan?",
    "Ja sam robot, ne viljuskar. Al evo ti ruka.",
    "Podizem. Spustam. Koja egzistencijalna svrha.",
    "Zasto sam ovdje? Samo da bih tebi pokazao kutove?"
};
const int numArm = 4;

const char* marvinQuotesSensor[] = {
    "Metal detektiran. Idem u mirovinu. Odmah.",
    "Senzor kaze nesto je tu. A mene boli briga.",
    "Ovakvi senzori osjetno vrjedaju moju inteligenciju.",
    "Udaljenost se smanjuje. Sudar je mozda neizbjezan."
};
const int numSensor = 4;

const char* marvinQuotesQR[] = {
    "Mozak velicine planeta, a ja citam QR kodove...",
    "Jos jedan QR kod. Bas nam je trebao u zivotu.",
    "Skeniram podatke koje me se ionako ne ticu.",
    "Nevidljive linije na zidu... to ja vidim."
};
const int numQR = 4;

int currentQuoteIdx = 0;
unsigned long lastQuoteChange = 0;
const unsigned long quoteInterval = 8000; // 8 sekundi po šali
const char* trenutnaSadrzanaSala = "Inicijalizacija boli.";

// Varijable za prioritetni prikaz (10 sekundi)
unsigned long alertStartTime = 0;
const unsigned long alertDuration = 10000;
String zadnjiPoznatiQR = "";
unsigned long qrPrikazTimer = 0;
bool qrAktivnoPrikazan = false;
bool bioMetal = false;

void inicijalizirajDisplay() {
    delay(250); // Kratka pauza za paljenje ekrana
    if(!display.begin(SCREEN_ADDRESS, true)) {
        Serial.println(F("SH1106 allocation failed"));
        return;
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0,0);
    display.println(F("Marvin OS v0.1"));
    display.println(F("Inicijalizacija..."));
    display.display();
    delay(1000);
}

void prikaziVelikiTekst(String tekst) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SH110X_WHITE);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(tekst, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w)/2, (64 - h)/2);
    display.println(tekst);
    display.display();
}

void azurirajDisplayCekanjeStarta(String ip) {
    display.clearDisplay();
    
    // Prikaz IP adrese na vrhu ekrana
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.print("IP: ");
    if (ip != "") {
        display.println(ip);
    } else {
        display.println("no comm");
    }

    // Prikaz CEKANJE STARTA
    String tekst = "CEKANJE STARTA";
    display.setTextSize(2);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(tekst, 0, 0, &x1, &y1, &w, &h);
    
    // Centriraj niže da ne prekrije IP
    display.setCursor((128 - w)/2, 25);
    display.println(tekst);
    
    display.display();
}

void azurirajDisplay(String qr, String pozicija, bool metal, long tof, char kameraMod) {
    unsigned long currentMillis = millis();
    
    display.clearDisplay();

    if (metal) {
        // Ako je metal detektiran, prvenstvo ima poruka LIMENKA
        display.setTextSize(3);
        display.setTextColor(SH110X_WHITE);
        display.setCursor(0, 15);
        display.println("LIMENKA");
    } else if (kameraMod == 'u' || pozicija == "SKENIRANJE...") {
        // AKO JE MOD KAMERE 'u' (Mjerenje udaljenosti TOF) ili skeniranje ruke
        display.setTextSize(2);
        display.setCursor(0, 10);
        display.println("Udaljenost:");
        
        display.setCursor(0, 35);
        if (tof >= 0) {
            display.print(tof);
            display.println(" mm");
        } else {
            display.println("--- mm");
        }
        display.display();
        return; // Prekida se daljnje crtanje, ekran ostaje čist s ovom informacijom
    }

    // Odabiranje kontekstualne šale (svakih 8 sekundi se mijenja tekst, iz kategorije)
    if (currentMillis - lastQuoteChange > quoteInterval) {
        lastQuoteChange = currentMillis;
        
        if (metal) {
            trenutnaSadrzanaSala = marvinQuotesSensor[random(0, numSensor)];
        } else if (pozicija != "PARKING" && pozicija != "SAFE" && pozicija != "") {
            // Ako se ruka miče aktivno po spremnicima
            trenutnaSadrzanaSala = marvinQuotesArm[random(0, numArm)];
        } else if (qr != "") {
            trenutnaSadrzanaSala = marvinQuotesQR[random(0, numQR)];
        } else {
            // Ništa pametno, baci generičnu depresiju
            trenutnaSadrzanaSala = marvinQuotesGeneral[random(0, numGeneral)];
        }
    }
    
    // Detekcija novog QR koda
    if (qr != "" && qr != zadnjiPoznatiQR) {
        zadnjiPoznatiQR = qr;
        qrPrikazTimer = currentMillis;
        qrAktivnoPrikazan = true;
    }
    
    // Ako je aktivan QR prikaz i nije isteklo 5 sekundi
    if (qrAktivnoPrikazan && (currentMillis - qrPrikazTimer <= 5000)) {
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("--- QR Ocitano ---");
        
        // Prikazi sirovi QR kod s velikim fontom ako stane, ili s pametnim wrapom
        // Cijeli ekran je posvećen samo QR kodu
        prikaziOmotanTekst(zadnjiPoznatiQR.c_str(), 0, 20, 128);
        display.display();
        return; // Preskoči ispis šala
    } else {
        qrAktivnoPrikazan = false; // Istekao timer
    }

    // Prikaz same šale uz pametni word-wrap sa standardnim sitnim fontom
    // Standardni font se crta s Top-Left pristupom (za razliku od baseline) pa spuštamo na 25
    prikaziOmotanTekst(trenutnaSadrzanaSala, 0, 25, 128);

    display.display();
}
