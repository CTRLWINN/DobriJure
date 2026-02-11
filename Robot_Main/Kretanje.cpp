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
    
    // Parametri za Ramp-Down (Usporavanje)
    // Zelimo usporavati zadnjih 20% puta ili zadnjih 15cm (sto je manje/vise ovisno o distanci)
    // Ovdje cemo koristiti fiksni threshold pulsa ako je distanca velika.
    long rampThreshold = cilj * 0.2; // Zadnjih 20%
    if (rampThreshold > (15 * IMPULSA_PO_CM)) rampThreshold = 15 * IMPULSA_PO_CM; // Max 15cm usporavanja
    
    int minBrzina = 60;  // Minimalna brzina da motori ne stanu prerano
    int maxBrzina = BAZNA_BRZINA; // Npr. 200

    while (true) {
        long encL = abs(dohvatiLijeviEnkoder());
        long encR = abs(dohvatiDesniEnkoder());
        long trenutniPut = (encL + encR) / 2;
        long preostalo = cilj - trenutniPut;

        // 1. Provjera kraja
        if (preostalo <= 0) {
            break;
        }
        
        // 2. Izracun Brzine (Ramp-Down Profil)
        int trenutnaBrzina = maxBrzina;
        
        if (preostalo < rampThreshold) {
            // Matematika usporavanja:
            // Mapiramo 'preostalo' (od threshold do 0) u raspon (maxBrzina do minBrzina)
            // Kad je preostalo == threshold -> brzina = maxBrzina
            // Kad je preostalo == 0         -> brzina = minBrzina
            trenutnaBrzina = map(preostalo, 0, rampThreshold, minBrzina, maxBrzina);
        }

        // 3. Sinkronizacija (P-regulator)
        // Error = Razlika medu enkoderima
        long error = encL - encR;
        int korekcija = error * Kp_Enc;
        
        // Limit korelcije da ne dominira nad usporavanjem
        if (korekcija > 30) korekcija = 30;
        if (korekcija < -30) korekcija = -30;
        
        int motL = trenutnaBrzina - korekcija;
        int motR = trenutnaBrzina + korekcija;
        
        // Limit PWM-a (i osiguraj da ne idemo ispod 0)
        motL = constrain(motL, 0, 255);
        motR = constrain(motR, 0, 255);
        
        // 4. Pogon
        if (smjer > 0) {
            lijeviMotor(motL);
            desniMotor(motR);
        } else {
            lijeviMotor(-motL);
            desniMotor(-motR);
        }
        
        delay(1); 
    }
    
    // 5. Aktivno kocenje
    motori_koci();
    delay(100); // Drzi kocnicu 100ms
    stani();    // Oslobodi motor (Coast / High Z)
}

// Ostatak funkcija (prazne ili implementirane kasnije)
// 4. Manualna vožnja
// Ako je cm > 0, vozi dok ne prijedje put (blokirajuce).
// Ako je cm == 0, samo pokreni motore (ne blokira).
void vozi(int l, int r, float cm) {
    // 1. Pokreni motore
    ::lijeviMotor(l);
    ::desniMotor(r);

    // 2. Ako nema distance, gotovi smo (non-blocking)
    if (cm <= 0) return;

    // 3. Ako ima distance, cekamo (blocking)
    resetirajEnkodere();
    long cilj = abs(cm * IMPULSA_PO_CM);
    
    // Safety timeout? Za sad ne, vjerujemo enkoderima.
    
    while (true) {
        long encL = abs(dohvatiLijeviEnkoder());
        long encR = abs(dohvatiDesniEnkoder());
        
        // Gledamo prosjek puta
        if ((encL + encR) / 2 >= cilj) {
            break;
        }
        
        delay(1);
    }
    
    stani();
}

void vozi(int l, int r) {
    vozi(l, r, 0);
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
