/**
 * SenzorBoje.cpp
 * 
 * Implementacija detekcije boje.
 */

#include "SenzorBoje.h"
#include <Wire.h>
#include <SparkFun_APDS9960.h> // Pretpostavljena biblioteka

SparkFun_APDS9960 apds = SparkFun_APDS9960();

void inicijalizirajSenzorBoje() {
    Serial.println("Senzor Boje Inicijalizacija...");
    
    // Inicijalizacija
    if (apds.init()) {
        Serial.println("Senzor boje spojen.");
    } else {
        Serial.println("GRESKA: Senzor boje nije pronaden!");
    }
    
    // Pokreni senzor svjetla (RGB)
    if (apds.enableLightSensor(false)) { // false = bez interrupta
        Serial.println("Senzor svjetla aktivan.");
    }
}

int citajBoju() {
    uint16_t ambijent, crvena, zelena, plava;
    
    // Čitaj vrijednosti
    if ( !apds.readAmbientLight(ambijent) ||
         !apds.readRedLight(crvena) ||
         !apds.readGreenLight(zelena) ||
         !apds.readBlueLight(plava) ) {
         return BOJA_NISTA;
    }

    // Detekcija "Ništa" ako je premračno ili nema objekta
    if (ambijent < 50) return BOJA_NISTA;

    // Jednostavna logika za diskretizaciju boja
    // Potrebno kalibrirati pragove prema stvarnom objektu i svjetlu
    
    // Crvena dominira
    if (crvena > plava && crvena > zelena && crvena > 100) {
        // Žuta je kombinacija Crvene i Zelene (R + G)
        if (zelena > (crvena * 0.6)) { 
             return BOJA_ZUTA;
        }
        return BOJA_CRVENA;
    }
    
    // Plava dominira
    if (plava > crvena && plava > zelena && plava > 100) {
        return BOJA_PLAVA;
    }

     // Dodatna provjera za Žutu ako je zelena jaka
    if (zelena > plava && crvena > plava && zelena > 100) {
        return BOJA_ZUTA; 
    }

    return BOJA_NISTA;
}
