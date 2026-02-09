/**
 * Motori.h
 * 
 * Modul za upravljanje pogonskim motorima.
 * Podržava diferencijalni pogon sa PWM brzinom (-255 do 255).
 */

#ifndef MOTORI_H
#define MOTORI_H

#include <Arduino.h>

/**
 * Postavlja brzinu lijevog motora.
 * @param brzina Vrijednost od -255 (nazad max) do 255 (naprijed max). 0 je stop.
 */
void lijeviMotor(int brzina);

/**
 * Postavlja brzinu desnog motora.
 * @param brzina Vrijednost od -255 (nazad max) do 255 (naprijed max). 0 je stop.
 */
void desniMotor(int brzina);

/**
 * Zaustavlja oba motora trenutno.
 */
void stani();

#endif // MOTORI_H
