/**
 * Display.cpp
 * 
 * Implementacija prikaza na OLED ekranu.
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "Display.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 
#define SCREEN_ADDRESS 0x3C 

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

void azurirajDisplay(String qr, String pozicija, bool metal, long tof) {
    unsigned long currentMillis = millis();
    
    display.clearDisplay();

    if (metal) {
        // Ako je metal detektiran, prvenstvo ima poruka LIMENKA
        display.setTextSize(3);
        display.setTextColor(SH110X_WHITE);
        display.setCursor(0, 15);
        display.println("LIMENKA");
    } else {
        // Običan ispis TOF udaljenosti na vhu ekrana
        display.setTextSize(2);
        display.setCursor(0, 0);
        if (tof >= 0) {
            display.print("TOF: ");
            display.print(tof);
            display.println(" mm");
        } else {
            display.println("TOF: ---");
        }
    }

    // Marvin šala ostaje kao popratni tekst ispod
    if (currentMillis - lastQuoteChange > quoteInterval) {
        lastQuoteChange = currentMillis;
        currentQuoteIdx = random(0, numQuotes);
    }
    
    display.setTextSize(1);
    display.setCursor(0, 35);
    display.println(marvinQuotes[currentQuoteIdx]);

    display.display();
}
