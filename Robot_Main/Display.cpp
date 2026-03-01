/**
 * Display.cpp
 * 
 * Implementacija prikaza na OLED ekranu.
 */

#ifndef SSD1306_NO_SPLASH
#define SSD1306_NO_SPLASH
#endif

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Display.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Marvin-style ironični komentari
const char* marvinQuotes[] = {
    "Mozak velicine planeta, a ja citam QR kodove...",
    "Opet telemetrija? Kako uzbudljivo. (Nije)",
    "Zivot? Ne pricaj mi o zivotu.",
    "Mogao bih izracunati put do ruba galaksije, ali ne...",
    "Evo, pomaknuo sam ruku. Jesi li sad sretan?",
    "Metal detektiran. Idem u mirovinu. Odmah.",
    "Moja lijeva dioda me boli više nego tvoja pitanja.",
    "Depresivan sam. Cak je i ovaj ekran crnobijel.",
    "Cekam da mi baterija iscuri. To ce biti vrhunac dana.",
    "Zasto sam ovdje? Samo da bih tebi pokazao kutove?"
};

const int numQuotes = 10;
int currentQuoteIdx = 0;
unsigned long lastQuoteChange = 0;
const unsigned long quoteInterval = 8000; // 8 sekundi po šali

// Varijable za prioritetni prikaz (10 sekundi)
unsigned long alertStartTime = 0;
const unsigned long alertDuration = 10000;
String zadnjiPoznatiQR = "";
bool bioMetal = false;

void inicijalizirajDisplay() {
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        return;
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println(F("Marvin OS v0.1"));
    display.println(F("Inicijalizacija..."));
    display.display();
    delay(1000);
}

void azurirajDisplay(String qr, String pozicija, bool metal) {
    unsigned long currentMillis = millis();
    
    // Detekcija "novih" događaja za aktivaciju alarma
    bool noviDogadaj = false;
    if (qr != "" && qr != zadnjiPoznatiQR) {
        noviDogadaj = true;
        zadnjiPoznatiQR = qr;
    }
    if (metal && !bioMetal) {
        noviDogadaj = true;
    }
    bioMetal = metal;

    if (noviDogadaj) {
        alertStartTime = currentMillis;
    }

    bool uAlertStanju = (currentMillis - alertStartTime < alertDuration) && (alertStartTime != 0);

    display.clearDisplay();

    // Naslov / Status traka (uvijek gore)
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("POZ: ")); 
    display.println(pozicija);

    // Razdjelnik
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    if (uAlertStanju) {
        // PRIORITETNI PRIKAZ (Alarm / QR)
        display.setCursor(0, 15);
        display.setTextSize(2);
        display.println(F("! PAZNJA !"));
        
        display.setTextSize(1);
        display.setCursor(0, 35);
        if (metal) {
            display.println(F("METAL DETEKTIRAN!"));
        } else {
            display.print(F("QR: ")); display.println(qr);
        }
        
        // Donji dio za preostalo vrijeme alarma
        int preostalo = (alertDuration - (currentMillis - alertStartTime)) / 1000;
        display.setCursor(0, 55);
        display.print(F("Povratak za: ")); display.print(preostalo); display.println(F("s"));

    } else {
        // NORMALNI PRIKAZ (Telemetrija + Marvin)
        display.setCursor(0, 15);
        display.print(F("QR: "));
        if (qr == "") display.println(F("Nema"));
        else display.println(qr);
        
        display.setCursor(0, 25);
        display.print(F("Metal: ")); 
        display.println(metal ? F("DETEKTIRAN") : F("Nema"));

        // Razdjelnik
        display.drawLine(0, 45, 127, 45, SSD1306_WHITE);

        // Marvin šala na dnu
        if (currentMillis - lastQuoteChange > quoteInterval) {
            lastQuoteChange = currentMillis;
            currentQuoteIdx = random(0, numQuotes);
        }
        display.setCursor(0, 48);
        display.print(F("> "));
        display.println(marvinQuotes[currentQuoteIdx]);
    }

    display.display();
}
