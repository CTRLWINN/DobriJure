#include "Kretanje.h"
#include "Motori.h"
#include "Enkoderi.h"
#include "IMU.h"

// Konfiguracija
float IMPULSA_PO_CM = 42.85;
int BAZNA_BRZINA = 100;
float Kp_Enc = 1.0; // Pojacanje P-regulatora za sinkronizaciju

void voziRavno(float cm) {
    if (cm == 0) return;

    resetirajEnkodere();
    
    long cilj = abs(cm * IMPULSA_PO_CM);
    int smjer = (cm > 0) ? 1 : -1;
    
    // Pocetna brzina
    int brzina = BAZNA_BRZINA;
    
    while (true) {
        long L = abs(dohvatiLijeviEnkoder());
        long R = abs(dohvatiDesniEnkoder());
        
        // 1. Provjera kraja
        if ((L + R) / 2 >= cilj) {
            break;
        }
        
        // 2. Sinkronizacija (P-regulator)
        // Error = Razlika medu enkoderima
        // Ako je L > R, error je pozitivan -> L treba usporiti.
        long error = L - R;
        int korekcija = error * Kp_Enc;
        
        // Limit korekcije da ne divlja
        if (korekcija > 50) korekcija = 50;
        if (korekcija < -50) korekcija = -50;
        
        int motL = brzina - korekcija;
        int motR = brzina + korekcija;
        
        // Limit PWM-a
        if (motL > 255) motL = 255; if (motL < 0) motL = 0;
        if (motR > 255) motR = 255; if (motR < 0) motR = 0;
        
        // 3. Pogon
        if (smjer > 0) {
            lijeviMotor(motL);
            desniMotor(motR);
        } else {
            lijeviMotor(-motL);
            desniMotor(-motR);
        }
        
        // Kratka pauza za stabilnost petlje (opcionalno, ali dobro za debounce enkodera u ispisu)
        delay(1); 
    }
    
    stani();
}

// Ostatak funkcija (prazne ili implementirane kasnije)
// 4. Manualna vožnja (bez distance)
void vozi(int lijeviMotor, int desniMotor) {
    // Koristimo scope resolution operator :: jer se argumenti zovu isto kao funkcije u Motori.h
    ::lijeviMotor(lijeviMotor);
    ::desniMotor(desniMotor);
}
// --- Funkcije za okretanje (koriste IMU/Žiroskop) ---

// Helper za ažuriranje IMU-a (ako nije dostupan preko headera)
extern void azurirajIMU(); 

void okreni(float kut) {
    if (kut == 0) return;
    
    // Resetiraj žiroskopsku orijentaciju
    resetirajGyro();
    
    // Ciljni kut. 
    // Pretpostavka: pozitivni kut = Desno (CW). 
    // Gyro obično vraća pozitivno za Lijevo (CCW). 
    // Zato cilj = -kut.
    float cilj = -kut; 
    
    int speed = BAZNA_BRZINA;
    int dirL = (kut > 0) ? 1 : -1; // Desno->L naprijed
    int dirR = (kut > 0) ? -1 : 1; // Desno->R nazad
    
    while(true) {
        azurirajIMU();
        float trenutni = dohvatiKutGyro();
        
        // Provjera
        if (kut > 0) { // Okret udesno (cilj je negativan)
            if (trenutni <= cilj) break;
        } else {       // Okret ulijevo (cilj je pozitivan)
            if (trenutni >= cilj) break;
        }
        
        lijeviMotor(speed * dirL);
        desniMotor(speed * dirR);
        delay(1);
    }
    stani();
}

void skreni(float kut) {
    if (kut == 0) return;
    resetirajGyro();
    
    float cilj = -kut;
    int speed = BAZNA_BRZINA;
    
    while(true) {
        azurirajIMU();
        float trenutni = dohvatiKutGyro();
        
        bool stigli = false;
        if (kut > 0 && trenutni <= cilj) stigli = true;
        if (kut < 0 && trenutni >= cilj) stigli = true;
        
        if (stigli) break;
        
        if (kut > 0) {
            // Desno: L vozi, R stoji
            lijeviMotor(speed);
            desniMotor(0);
        } else {
            // Lijevo: R vozi, L stoji
            lijeviMotor(0);
            desniMotor(speed);
        }
        delay(1);
    }
    stani();
}
void postaviKonfigKretanja(float i) { IMPULSA_PO_CM = i; }
void stani() {
    motori_stani();
}

void zaustaviKretanje() { 
    stani(); 
}
