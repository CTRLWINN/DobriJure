/**
 * HardwareMap.h
 * 
 * Centralna datoteka za mapiranje hardverskih pinova i definicija.
 * Sadrži definicije svih pinova prema specifikaciji.
 */

#ifndef HARDWAREMAP_H
#define HARDWAREMAP_H

#include <Arduino.h>

// --- POGON (L298N ili slično) ---
#define PIN_MOTOR_L_PWM 12
#define PIN_MOTOR_L_IN1 8
#define PIN_MOTOR_L_IN2 7

#define PIN_MOTOR_R_PWM 11
#define PIN_MOTOR_R_IN1 6
#define PIN_MOTOR_R_IN2 5

// --- ENKODERI (Kvadraturni) ---
// Lijevi Enkoder
#define PIN_ENC_L_A 18 // Interrupt
#define PIN_ENC_L_B 19 // Interrupt

// Desni Enkoder
#define PIN_ENC_R_A 2  // Interrupt
#define PIN_ENC_R_B 3  // Interrupt

// --- KOMUNIKACIJA ---
// Serial0 (USB Debug) - definiran u Arduinu kao Serial
// Serial2 (Bluetooth / Dashboard)
#define PIN_BT_RX 17
#define PIN_BT_TX 16
// Serial3 (AI Kamera / Vision)
#define PIN_VISION_RX 15
#define PIN_VISION_TX 14

// --- LANE ASSIST (Analogni IR na bumperima) ---
#define PIN_IR_LIJEVI A0    // Analog Input (lijevi bumper)
#define PIN_IR_DESNI  A1    // Analog Input (desni bumper)

// --- ULTRAZVUČNI SENZORI (HC-SR04) ---
// Prednji
#define PIN_US_FRONT_TRIG 30
#define PIN_US_FRONT_ECHO 31
// Stražnji
#define PIN_US_BACK_TRIG  32
#define PIN_US_BACK_ECHO  33
// Lijevi
#define PIN_US_LEFT_TRIG  34
#define PIN_US_LEFT_ECHO  35
// Desni
#define PIN_US_RIGHT_TRIG 36
#define PIN_US_RIGHT_ECHO 37

// --- SMART GRIPPER (Ultrazvuk za hvataljku) ---
#define PIN_ULTRAZVUK_GRIPPER_TRIG 38
#define PIN_ULTRAZVUK_GRIPPER_ECHO 39

// --- OSTALO ---
#define PIN_INDUCTIVE_SENS 4 // Digital Input (Induktivni)
#define PIN_SERVO_GRIP     44
#define PIN_SERVO_LIFT     45

// --- I2C SENZORI (IMU & Boja) ---
// Spojeni na SDA(20) i SCL(21)
// IMU Adresa: 0x1E
// Boja Adresa: 0x39

// --- MANIPULATOR (PCA9685) ---
#include <Adafruit_PWMServoDriver.h>

// I2C Adresa PCA9685 drivera
#define ADRESA_PCA9685 0x40

// Objekt PWM drivera (deklaracija)
extern Adafruit_PWMServoDriver pwm;

// Kanali (Hrvatski)
#define KANAL_BAZA         0 // Base/Waist
#define KANAL_RAME         1 // Shoulder
#define KANAL_LAKAT        2 // Elbow
#define KANAL_PODLAKTICA   3 // Forearm
#define KANAL_ZGLOB_ROT    4 // Wrist Rotation
#define KANAL_ZGLOB_NAGIB  5 // Wrist Pitch
#define KANAL_HVATALJKA    6 // Gripper

// --- KONFIGURACIJA STRUCT (EEPROM) ---
struct ServoPreset {
    float angles[6]; // Kutevi za 6 serva
};

struct RobotConfig {
    // PID
    float kp;
    float ki;
    float kd;
    
    // Motor Config
    float pulsesPerCm;
    int baseSpeed;
    
    // Servo Presets (15 slotova)
    ServoPreset presets[15];
    
    // Boje 
    int colors[3][6]; 
    
    int magic; // 0x1234
};

extern RobotConfig config;
extern const char* presetNames[];

/**
 * Inicijalizira sve pinove na potrebne modove (INPUT/OUTPUT).
 * Pokreće serijske portove i I2C sabirnicu.
 */
void inicijalizirajHardware();

#endif // HARDWAREMAP_H
