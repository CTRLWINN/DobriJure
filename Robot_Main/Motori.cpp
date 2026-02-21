/**
 * Motori.cpp
 * 
 * Implementacija upravljanja motorima.
 */

#include "Motori.h"
#include "HardwareMap.h"

// Pomoćna funkcija za postavljanje pinova pojedinog motora
void postaviMotor(int pinPWM, int pinIN1, int pinIN2, int brzina) {
    // Ograniči brzinu na dozvoljeni raspon
    brzina = constrain(brzina, -255, 255);

    if (brzina > 0) {
        // Naprijed
        digitalWrite(pinIN1, HIGH);
        digitalWrite(pinIN2, LOW);
        analogWrite(pinPWM, brzina);
    } else if (brzina < 0) {
        // Nazad
        digitalWrite(pinIN1, LOW);
        digitalWrite(pinIN2, HIGH);
        analogWrite(pinPWM, abs(-brzina)); // PWM mora biti pozitivan
    } else {
        // Stop (kočenje)
        digitalWrite(pinIN1, LOW);
        digitalWrite(pinIN2, LOW);
        analogWrite(pinPWM, 0);
    }
}

void lijeviMotor(int brzina) {
    postaviMotor(PIN_MOTOR_L_PWM, PIN_MOTOR_L_IN1, PIN_MOTOR_L_IN2, brzina);
}

void desniMotor(int brzina) {
    postaviMotor(PIN_MOTOR_R_PWM, PIN_MOTOR_R_IN1, PIN_MOTOR_R_IN2, brzina);
}

void motori_stani() {
    lijeviMotor(0);
    desniMotor(0);
}

void motori_koci() {
    // Lijevi - Short Brake (HIGH, HIGH)
    digitalWrite(PIN_MOTOR_L_IN1, HIGH);
    digitalWrite(PIN_MOTOR_L_IN2, HIGH);
    analogWrite(PIN_MOTOR_L_PWM, 255); // PWM nebitan kod Short Brake na nekim driverima, ali MAX je sigurno
    
    // Desni - Short Brake (HIGH, HIGH)
    digitalWrite(PIN_MOTOR_R_IN1, HIGH);
    digitalWrite(PIN_MOTOR_R_IN2, HIGH);
    analogWrite(PIN_MOTOR_R_PWM, 255); 
}
