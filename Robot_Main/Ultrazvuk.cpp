/**
 * Ultrazvuk.cpp
 * 
 * Implementacija mjerenja udaljenosti.
 */

#include "Ultrazvuk.h"
#include "HardwareMap.h"

// Pomoćna funkcija za raw mjerenje
long izmjeri(int trigPin, int echoPin) {
    // Očisti trig pin
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    
    // Pošalji impuls od 10us
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    // Čitaj echo impuls (timeout 30ms ~ 5m)
    long duration = pulseIn(echoPin, HIGH, 30000);
    
    if (duration == 0) return -1; // Timeout
    
    // Zvuk putuje 343m/s -> 0.0343 cm/us
    // Udaljenost = (vrijeme * brzina) / 2
    return (duration * 0.0343) / 2;
}

long udaljenost(int smjer) {
    switch(smjer) {
        case SMJER_NAPRIJED:
            return izmjeri(PIN_US_FRONT_TRIG, PIN_US_FRONT_ECHO);
        case SMJER_NAZAD:
            return izmjeri(PIN_US_BACK_TRIG, PIN_US_BACK_ECHO);
        case SMJER_LIJEVO:
            return izmjeri(PIN_US_LEFT_TRIG, PIN_US_LEFT_ECHO);
        case SMJER_DESNO:
            return izmjeri(PIN_US_RIGHT_TRIG, PIN_US_RIGHT_ECHO);
        case SMJER_GRIPPER:
            return izmjeri(PIN_ULTRAZVUK_GRIPPER_TRIG, PIN_ULTRAZVUK_GRIPPER_ECHO);
        default:
            return -1;
    }
}
