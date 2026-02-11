#include "Kretanje.h"
#include "Motori.h"
#include "Enkoderi.h"
#include "IMU.h"

// Konfiguracija
// Konfiguracija
float IMPULSA_PO_CM = 45;
float IMPULSA_PO_STUPNJU = 5.9375;
int BAZNA_BRZINA = 200;
int MIN_BRZINA = 80;
float Kp_Enc = 2.0; // Pojacanje P-regulatora za sinkronizaciju

void voziRavno(float cm, int speed) {
    if (cm == 0) return;

    resetirajEnkodere();
    
    long cilj = abs(cm * IMPULSA_PO_CM);
    int smjer = (cm > 0) ? 1 : -1;
    
    // Parametri za Ramp-Down (Usporavanje)
    // Zelimo usporavati zadnjih 20% puta ili zadnjih 15cm (sto je manje/vise ovisno o distanci)
    // Ovdje cemo koristiti fiksni threshold pulsa ako je distanca velika.
    long rampThreshold = cilj * 0.2; // Zadnjih 20%
    if (rampThreshold > (15 * IMPULSA_PO_CM)) rampThreshold = 15 * IMPULSA_PO_CM; // Max 15cm usporavanja
    
    // int minBrzina = 80;  // Use global MIN_BRZINA
    int maxBrzina = (speed > 0) ? speed : BAZNA_BRZINA; // Use passed speed or default

    unsigned long pocetak = millis();
    unsigned long zadnjiPomak = millis();
    long zadnjaPozicija = 0;
    
    // Timeout: Ako voznja traje predugo (npr. ocekivano + 3 sekunde)
    // Ocekivano ~ cm / (BAZNA_BRZINA scaled). 
    // Hard limit: 10 sekundi za debug
    unsigned long timeout = 10000; 

    while (true) {
        long encL = abs(dohvatiLijeviEnkoder());
        long encR = abs(dohvatiDesniEnkoder());
        long trenutniPut = (encL + encR) / 2;
        long preostalo = cilj - trenutniPut;

        // 1. Provjera kraja
        if (preostalo <= 0) {
            break;
        }
        
        // Safety: Emergency Stop from Dashboard
        if (provjeriHitniStop()) break;
        
        // Safety: Timeout
        if (millis() - pocetak > timeout) {
             break;
        }
        
        // Safety: Stall Detection (ako se ne micemo 500ms)
        if (abs(trenutniPut - zadnjaPozicija) > (IMPULSA_PO_CM / 2)) {
            zadnjaPozicija = trenutniPut;
            zadnjiPomak = millis();
        } else {
            if (millis() - zadnjiPomak > 1000) { // 1 sec bez pomaka
                break;
            }
        }
        
        // 2. Izracun Brzine (Ramp-Down Profil)
        int trenutnaBrzina = maxBrzina; // Defaultna brzina ako nismo u ramp-downu
        if (preostalo < rampThreshold) {
            // Matematika usporavanja:
            // Mapiramo 'preostalo' (od threshold do 0) u raspon (maxBrzina do minBrzina)
            // Kad je preostalo == threshold -> brzina = maxBrzina
            // Kad je preostalo == 0         -> brzina = minBrzina
            trenutnaBrzina = map(preostalo, 0, rampThreshold, MIN_BRZINA, maxBrzina);
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
    delay(500); // Drzi kocnicu 500ms (Povecano radi inercije)
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
        // Gledamo prosjek puta
        if ((encL + encR) / 2 >= cilj) {
            break;
        }
        
        if (provjeriHitniStop()) break;
        
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

void okreni(float kut, int speedArg) {
    if (kut == 0) return;
    
    resetirajEnkodere();
    
    // 5.9375 impulsa po stupnju
    long cilj = abs(kut * IMPULSA_PO_STUPNJU);
    int speed = (speedArg > 0) ? speedArg : BAZNA_BRZINA;
    
    unsigned long pocetak = millis();
    unsigned long zadnjiPomak = millis();
    long zadnjaPozicija = 0;
    unsigned long timeout = 10000;
    
    // kut > 0 -> Desno (L naprijed, D nazad)
    // kut < 0 -> Lijevo (L nazad, D naprijed)
    int dirL = (kut > 0) ? 1 : -1;
    int dirR = (kut > 0) ? -1 : 1;

    while(true) {
        long encL = abs(dohvatiLijeviEnkoder());
        long encR = abs(dohvatiDesniEnkoder());
        
        // Prosjek oba kotaca (apsolutni put)
        long trenutni = (encL + encR) / 2;
        
        if (trenutni >= cilj) break;
        
        if (provjeriHitniStop()) break;
        
        // Safety: Timeout
        if (millis() - pocetak > timeout) break;
        
        // Safety: Stall Detection
        if (abs(trenutni - zadnjaPozicija) > (IMPULSA_PO_STUPNJU * 5)) {
            zadnjaPozicija = trenutni;
            zadnjiPomak = millis();
        } else {
             if (millis() - zadnjiPomak > 1000) break; 
        }
        
        lijeviMotor(speed * dirL);
        desniMotor(speed * dirR);
        delay(1);
    }
    stani();
}

void skreni(float kut, int speedArg) {
    if (kut == 0) return;
    
    resetirajEnkodere();
    
    long cilj = abs(kut * IMPULSA_PO_STUPNJU);
    // kut > 0 -> Desno (Lijevi vozi)
    // kut < 0 -> Lijevo (Desni vozi)
    
    int speed = (speedArg > 0) ? speedArg : BAZNA_BRZINA;
    
    unsigned long pocetak = millis();
    unsigned long zadnjiPomak = millis();
    long zadnjaPozicija = 0;
    unsigned long timeout = 10000;

    while(true) {
        long encL = abs(dohvatiLijeviEnkoder());
        long encR = abs(dohvatiDesniEnkoder());
        
        // Koji enkoder pratimo?
        long trenutni = (kut > 0) ? encL : encR;
        
        if (trenutni >= cilj) break;
        
        if (provjeriHitniStop()) break;
        
        // Safety
        if (millis() - pocetak > timeout) break;
        
        if (abs(trenutni - zadnjaPozicija) > (IMPULSA_PO_STUPNJU * 5)) {
            zadnjaPozicija = trenutni;
            zadnjiPomak = millis();
        } else {
             if (millis() - zadnjiPomak > 1000) break; 
        }
        
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


void postaviParametre(float imp, int brzina, float kp, int min_brzina) {
    if (imp > 0) IMPULSA_PO_CM = imp;
    if (brzina > 0) BAZNA_BRZINA = brzina;
    if (kp >= 0) Kp_Enc = kp;
    if (min_brzina >= 0) MIN_BRZINA = min_brzina;
}

bool provjeriHitniStop() {
    if (Serial2.available()) {
        String msg = Serial2.readString(); // Quick read due to setTimeout(5)
        if (msg.indexOf("stop") >= 0) {
            stani();
            return true;
        }
    }
    return false;
}
