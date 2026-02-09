/**
 * Hvataljka.cpp
 * 
 * Implementacija upravljanja servima.
 */

#include "Hvataljka.h"
#include "HardwareMap.h"

Servo servoGrip;
Servo servoLift;

// Konstante za pozicije serva (prilagoditi nakon kalibracije)
#define GRIP_OTVORENO 90
#define GRIP_ZATVORENO 10

#define LIFT_GORE 150
#define LIFT_DOLJE 30

void inicijalizirajHvataljku() {
    servoGrip.attach(PIN_SERVO_GRIP);
    servoLift.attach(PIN_SERVO_LIFT);
    
    // Postavi početno stanje
    otvori();
    liftGore();
}

void otvori() {
    servoGrip.write(GRIP_OTVORENO);
}

void zatvori() {
    servoGrip.write(GRIP_ZATVORENO);
}

void liftGore() {
    servoLift.write(LIFT_GORE);
}

void liftDolje() {
    servoLift.write(LIFT_DOLJE);
}
