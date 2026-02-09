/**
 * Hvataljka.h
 * 
 * Modul za upravljanje servo motorima hvataljke (kliješta) i lifta.
 */

#ifndef HVATALJKA_H
#define HVATALJKA_H

#include <Arduino.h>
#include <Servo.h>

/**
 * Inicijalizacija servo motora. Mora se pozvati u setup().
 */
void inicijalizirajHvataljku();

// Kliješta
void otvori();
void zatvori();

// Lift
void liftGore();
void liftDolje();

#endif // HVATALJKA_H
