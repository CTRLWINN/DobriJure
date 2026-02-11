/**
 * HardwareMap.cpp
 * 
 * Implementacija inicijalizacije hardvera.
 */

#include "HardwareMap.h"
#include <Wire.h>

// Instanca PWM drivera
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(ADRESA_PCA9685);

// Instanca konfiguracije
RobotConfig config;

// Imena preseta (usklađeno s Dashboardom)
const char* presetNames[] = {
    "Parkiraj", "Voznja", "Uzmi_Boca", "Uzmi_Limenka", "Uzmi_Spuzva",
    "Spremi_1", "Spremi_2", "Spremi_3",
    "Iz_Sprem_1", "Iz_Sprem_2", "Iz_Sprem_3",
    "Dostava_1", "Dostava_2", "Dostava_3", "Extra"
};

void inicijalizirajHardware() {
    // --- Inicijalizacija Serijskih Portova ---
    Serial.begin(115200);  // Debug
    Serial2.begin(9600);   // Bluetooth (Standardni baudrate za HC-05/06 je često 9600, prilagoditi po potrebi)
    Serial3.begin(115200); // Vision (Nicla Vision obično radi na višem baudrateu)
    
    Serial.println("Inicijalizacija hardvera...");

    // --- I2C ---
    Wire.begin();
    Wire.setClock(400000); // 400kHz Fast Mode

    // --- Inicijalizacija Manipulatora (PWM Driver) ---
    pwm.begin();
    pwm.setPWMFreq(50); // 50Hz za servo motore

    // --- Motori ---
    pinMode(PIN_MOTOR_L_PWM, OUTPUT);
    pinMode(PIN_MOTOR_L_IN1, OUTPUT);
    pinMode(PIN_MOTOR_L_IN2, OUTPUT);
    
    pinMode(PIN_MOTOR_R_PWM, OUTPUT);
    pinMode(PIN_MOTOR_R_IN1, OUTPUT);
    pinMode(PIN_MOTOR_R_IN2, OUTPUT);

    // --- Enkoderi ---
    // Pinovi za enkodere su INPUT_PULLUP za stabilnost signala
    pinMode(PIN_ENC_L_A, INPUT_PULLUP);
    pinMode(PIN_ENC_L_B, INPUT_PULLUP);
    pinMode(PIN_ENC_R_A, INPUT_PULLUP);
    pinMode(PIN_ENC_R_B, INPUT_PULLUP);

    // --- Senzori Linije (Stari niz uklonjen) ---
    // A0 i A1 se koriste za Lane Assist (Analogni), ne trebaju pinMode INPUT za analogRead,
    // ali za svaki slučaj (neki MCU traže):
    pinMode(PIN_IR_LIJEVI, INPUT);
    pinMode(PIN_IR_DESNI,  INPUT);

    // --- Ultrazvučni Senzori ---
    // Prednji
    pinMode(PIN_US_FRONT_TRIG, OUTPUT);
    pinMode(PIN_US_FRONT_ECHO, INPUT);
    // Stražnji
    pinMode(PIN_US_BACK_TRIG, OUTPUT);
    pinMode(PIN_US_BACK_ECHO, INPUT);
    // Lijevi
    pinMode(PIN_US_LEFT_TRIG, OUTPUT);
    pinMode(PIN_US_LEFT_ECHO, INPUT);
    // Desni
    pinMode(PIN_US_RIGHT_TRIG, OUTPUT);
    pinMode(PIN_US_RIGHT_ECHO, INPUT);

    // --- Smart Gripper ---
    pinMode(PIN_ULTRAZVUK_GRIPPER_TRIG, OUTPUT);
    pinMode(PIN_ULTRAZVUK_GRIPPER_ECHO, INPUT);

    // --- Lane Assist ---
    // Inicijalizirano gore (A0, A1)

    // --- Ostalo ---
    pinMode(PIN_INDUCTIVE_SENS, INPUT_PULLUP); // Pretpostavka open-collector ili slično
    // Servo pinovi ne trebaju eksplicitni pinMode ako koristimo Servo biblioteku, 
    // ali dobro ih je definirati ako se biblioteka ne koristi odmah.
    
    Serial.println("Hardver inicijaliziran.");
}

// Helper za UZ
long izmjeriUZ(int trig, int echo) {
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);
    
    long duration = pulseIn(echo, HIGH, 30000); // 30ms timeout (~5m)
    if (duration == 0) return -1; // Timeout
    return duration * 0.034 / 2;
}

long ocitajPrednjiUZ() { return izmjeriUZ(PIN_US_FRONT_TRIG, PIN_US_FRONT_ECHO); }
long ocitajStraznjiUZ() { return izmjeriUZ(PIN_US_BACK_TRIG, PIN_US_BACK_ECHO); }
long ocitajLijeviUZ()  { return izmjeriUZ(PIN_US_LEFT_TRIG, PIN_US_LEFT_ECHO); }
long ocitajDesniUZ()   { return izmjeriUZ(PIN_US_RIGHT_TRIG, PIN_US_RIGHT_ECHO); }

bool ocitajInduktivni() {
    // Induktivni senzor (NPN) daje LOW kada detektira metal
    return digitalRead(PIN_INDUCTIVE_SENS) == LOW; 
}
