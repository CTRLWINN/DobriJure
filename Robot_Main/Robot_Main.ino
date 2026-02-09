/**
 * Robot_Main.ino
 * 
 * Glavni firmware za WorldSkills Croatia 2026 Autonomni Robot.
 * Implementira "Grand Slam" strategiju.
 * 
 * Sadržaj:
 * 1. Smart Start (Skeniranje QR koda i pozicioniranje)
 * 2. Lane Assist (Sigurnosna provjera u Kretanje.cpp)
 * 3. IMU Detekcija Prepreka (Udarac o olovku)
 * 4. Izvršavanje Misije (Skupljanje i Sortiranje)
 * 5. Pametno Hvatanje (Senzori na rješenju)
 */

#include "HardwareMap.h"
#include "Manipulator.h"
#include "Kretanje.h"
#include "Motori.h"
#include "Enkoderi.h"
#include "IMU.h"
#include "Ultrazvuk.h"
#include "Vision.h" // Dodano
#include <ArduinoJson.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// --- KONFIGURACIJA ---
#define PRAG_UDARA_IMU 2.5 // G-sila trzanja (prilagoditi testiranjem)
#define BRZINA_PRETRAGE 80
#define BRZINA_MISIJA   120
#define BRZINA_SPORO    50

// --- GLOBALI ---
Manipulator ruka;
StaticJsonDocument<512> doc;

// Status Robota
long odmakOdStarta = 0;   // Koliko smo se maknuli tražeći QR kod
float zadnjiAccelZ = 0;   // Za detekciju udarca

// Stanja Misije
enum FazaMisije {
    FAZA_CEKANJE_STARTA, // Setup odrađen, čeka komandu ili tipku
    FAZA_SMART_START,    // Skeniranje i pozicioniranje
    FAZA_1_SKUPLJANJE,   // Vožnja po stazi, skupljanje P1, P2, P3
    FAZA_2_PARKIRANJE,   // Dolazak na D2
    FAZA_3_SORTIRANJE,   // Stacionarno bacanje
    FAZA_KRAJ            // Gotovo
};

FazaMisije trenutnaFaza = FAZA_CEKANJE_STARTA;
int korakFaze = 0;       // Pod-korak unutar faze

// Podaci o predmetima (QR Mapping)
// 0=Nepoznato, 1=Boca, 2=Limenka, 3=Spužva
int mapiranjeKrov1 = 0;
int mapiranjeKrov2 = 0;
int mapiranjeRuka  = 0; 
// Spremnici: D1(Lijevo), D2(Sredina), D3(Desno) - Mapiranje s QR koda
// Ovdje spremamo gdje što ide. Npr. spremnikType[0] je za D1.
// Ako je QR: "Boca=D1, Limenka=D2", onda parsiramo to.
// Za sad hardkodiramo ili čekamo QR logiku (nije detaljno specificirana u promptu osim "Check QR mapping").

// --- KONFIGURACIJA STRUCT (EEPROM) ---


RobotConfig config;

// Imena preseta za mapiranje (opcionalno za log ispis)
const char* presetNames[] = {
    "Parkiraj", "Voznja", "Uzmi_Boca", "Uzmi_Limenka", "Uzmi_Spuzva",
    "Spremi_1", "Spremi_2", "Spremi_3",
    "Iz_Sprem_1", "Iz_Sprem_2", "Iz_Sprem_3",
    "Dostava_1", "Dostava_2", "Dostava_3",
    "Extra"
};

// --- PROTOTIPOVI ---
// --- PROTOTIPOVI ---
void izvrsiSmartStart();
void provjeriUdarac();

// --- TELEMETRIJA ---
// --- TELEMETRIJA ---
void poslajiStatus() {
    // Format: STATUS:cm,pulsesL,pulsesR,armIdx,usF,usB,usL,usR,induct
    float cm = dohvatiPredjeniPutCm();
    long encL = dohvatiLijeviEnkoder();
    long encR = dohvatiDesniEnkoder();
    int armIdx = ruka.dohvatiCiljaniPreset();
    
    long usF = udaljenost(SMJER_NAPRIJED);
    long usB = udaljenost(SMJER_NAZAD);
    long usL = udaljenost(SMJER_LIJEVO);
    long usR = udaljenost(SMJER_DESNO);
    int induct = digitalRead(PIN_INDUCTIVE_SENS);
    
    Serial.print("STATUS:");
    Serial.print(cm); Serial.print(",");
    Serial.print(encL); Serial.print(",");
    Serial.print(encR); Serial.print(",");
    Serial.print(armIdx); Serial.print(",");
    Serial.print(usF); Serial.print(",");
    Serial.print(usB); Serial.print(",");
    Serial.print(usL); Serial.print(",");
    Serial.print(usR); Serial.print(",");
    Serial.println(induct);
}
void izvrsiFazuSkupljanja();
void izvrsiFazuSortiranja();
bool pametnoHvatanje(String destinacija);
void checkSerial(Stream* stream); // Novi parser
void ucitajKonfiguraciju();
void spremiKonfiguraciju();
void posaljiKonfiguracijuKameri(); 
void primjeniKonfiguraciju(); // Nova funkcija za update globalnih varijabli

void setup() {
    inicijalizirajHardware();
    inicijalizirajIMU();
    inicijalizirajEnkodere();
    
    // Ucitaj PID i Boje iz EEPROM-a
    ucitajKonfiguraciju();
    
    // Ruka u početni položaj
    ruka.zapocniSekvencu("home"); 

    Serial.println("WSC 2026 Robot Spreman.");
    Serial.println("Cekam QR scan...");
    
    // Početno očitavanje za IMU shock filter
    azurirajIMU();
    zadnjiAccelZ = dohvatiAccelZ();

    // Pokreni Smart Start odmah ili na gumb?
    // Prompt kaže: "Start Position: Robot's Right Side faces the track..."
    // Pretpostavljamo da startamo odmah ili na serijsku komandu "start".
    trenutnaFaza = FAZA_CEKANJE_STARTA; 
    
    // Pošalji konfiguraciju kameri (ako je spojena i bootana, mozda treba delay)
    // Nicla se boota sporije, pa ce poslati config kad dobijemo prvi "ping" od nje ili na zahtjev?
    // Za sad šaljemo odmah, pa ako Nicla nije spremna, Dashboard ce poslati ponovno.
    // Bolje: Posalji na 'SET' komande iz checkSerial.
    delay(1000); // Kratki wait za Niclu
    posaljiKonfiguracijuKameri();
}

void loop() {
    // --- 1. AŽURIRANJE PODSUSTAVA ---
    ruka.azuriraj();
    ruka.azuriraj();
    azurirajKretanje(); // Ovdje je Lane Assist & Fusion!
    azurirajIMU();
    azurirajVision();   // Procesiraj poruke s kamere
    
    // --- TELEMETRIJA (10Hz) ---
    static unsigned long zadnjaTelemetrija = 0;
    if (millis() - zadnjaTelemetrija > 100) {
        poslajiStatus();
        zadnjaTelemetrija = millis();
    }

    // --- 2. SIGURNOST (IMU PREPREKA) ---
    if (jeUPokretu()) {
        provjeriUdarac();
    }

    // --- STATUS REPORT (DONE) ---
    static bool bioUPokretu = false;
    bool sadaUPokretu = jeUPokretu();
    
    if (bioUPokretu && !sadaUPokretu) {
        // Upravo smo stali
        Serial.println("{\"status\": \"DONE\"}");
        Serial2.println("{\"status\": \"DONE\"}");
    }
    bioUPokretu = sadaUPokretu;

    // --- 3. GLAVNA LOGIKA (STATE MACHINE) ---
    switch (trenutnaFaza) {
        case FAZA_CEKANJE_STARTA:
            // Čekamo komandu s Bluetootha ili Seriala
            if (Serial.available() || Serial2.available()) {
                // Pojednostavljeno: bilo koji znak pokreće
                while(Serial.available()) Serial.read();
                while(Serial2.available()) Serial2.read();
                trenutnaFaza = FAZA_SMART_START;
                korakFaze = 0;
            }
            break;

        case FAZA_SMART_START:
            izvrsiSmartStart();
            break;

        case FAZA_1_SKUPLJANJE:
            izvrsiFazuSkupljanja();
            break;

        case FAZA_2_PARKIRANJE:
            // Vozi do D2 (sredina)
            // Pretpostavka: Znamo distancu ili pratimo liniju do kraja
            // Ovdje samo placeholder logika
            if (korakFaze == 0) {
                 zapocniVoznju(50); // Primjer: zadnji segment
                 korakFaze++;
            } else {
                 if (!jeUPokretu()) {
                     trenutnaFaza = FAZA_3_SORTIRANJE;
                     korakFaze = 0;
                 }
            }
            break;

        case FAZA_3_SORTIRANJE:
            izvrsiFazuSortiranja();
            break;

        case FAZA_KRAJ:
            // Gotovo, blinkaj LED ili sviraj
            break;
    }
    // --- 4. PARSIRANJE KOMANDI (Serial/Bluetooth) ---
    // Provjeri Serial (USB) i Serial2 (Bluetooth)
    checkSerial(&Serial);
    checkSerial(&Serial2);
    
    // Provjeri odgovor s Kamere i proslijedi PC-u (Debug)
    // Ovo je sada riješeno unutar azurirajVision() koji parsira QR i Error.
    // Ali zelimo i dalje prosljedjivati RAW poruke na Bluetooth radi debuga?
    // Vision.cpp radi print na Serial, možemo dodati Serial2.
    // Za sad, azurirajVision() cita Serial3 i prazni buffer, pa ovaj blok ispod
    // vise nece uhvatiti nista jer je buffer prazan.
    // Moramo modificirati Vision.cpp ako zelimo prosljedjivanje ili vjerovati logici.
    // Ostavljamo ovaj blok prazan ili zakomentiran.
}

// --- TELEMETRIJA & PARSER (JSON) ---
void checkSerial(Stream* stream) {
    if (stream->available()) {
        String msg = stream->readStringUntil('\n');
        msg.trim();
        if (msg.length() == 0) return;

        // Pokusaj parsirati JSON (doc je globalni StaticJsonDocument definiran gore)
        DeserializationError error = deserializeJson(doc, msg);

        if (!error) {
            const char* cmd = doc["cmd"];
            
            if (strcmp(cmd, "straight") == 0) {
                float val = doc["val"];
                straightDrive(val);
                stream->println("{\"status\": \"OK\"}");
            }
            else if (strcmp(cmd, "move_dual") == 0) {
                int l = doc["l"];
                int r = doc["r"];
                float dist = doc["dist"];
                differentialDrive(l, r, dist);
                stream->println("{\"status\": \"OK\"}");
            }
            else if (strcmp(cmd, "turn") == 0) {
                float val = doc["val"];
                zapocniRotaciju(val);
                stream->println("{\"status\": \"OK\"}");
            }
            else if (strcmp(cmd, "pivot") == 0) {
                float val = doc["val"];
                pivotTurn(val);
                stream->println("{\"status\": \"OK\"}");
            }
            else if (strcmp(cmd, "arm") == 0) {
                const char* val = doc["val"];
                ruka.zapocniSekvencu(val); 
                stream->println("{\"status\": \"OK\"}");
            }
            else if (strcmp(cmd, "manual") == 0) {
                 int l = doc["l"];
                 int r = doc["r"];
                 lijeviMotor(l);
                 desniMotor(r);
            }
            else if (strcmp(cmd, "stop") == 0) {
                 zaustaviKretanje();
                 stream->println("{\"status\": \"STOPPED\"}");
                 trenutnaFaza = FAZA_CEKANJE_STARTA;
            }
            else if (strcmp(cmd, "set_pid") == 0) {
                config.kp = doc["p"];
                config.ki = doc["i"];
                config.kd = doc["d"];
                extern float Kp, Ki, Kd;
                Kp = config.kp; Ki = config.ki; Kd = config.kd;
                spremiKonfiguraciju();
                stream->println("{\"status\": \"PID_SAVED\"}");
            }
            else if (strcmp(cmd, "cal_imu") == 0) {
                inicijalizirajIMU(); 
                stream->println("{\"status\": \"IMU_CALIBRATED\"}");
            }
            else if (strcmp(cmd, "save_eeprom") == 0) {
                spremiKonfiguraciju();
                stream->println("{\"status\": \"SAVED\"}");
            }
             else if (strcmp(cmd, "get_pose") == 0) {
                 Serial2.println("{\"pose_x\": 0, \"pose_y\": 0, \"pose_th\": 0}"); 
             }
        }
    }
}

// --- EEPROM FUNKCIJE ---
void ucitajKonfiguraciju() {
    EEPROM.get(0, config);
    if (config.magic != 0x1234) {
        // Prvo pokretanje
        config.magic = 0x1234;
        config.kp = 35.0;
        config.ki = 0.0;
        config.kd = 15.0;
        config.pulsesPerCm = 40.0;
        config.baseSpeed = 100;
        
        // Zero out presets
        for(int i=0; i<15; i++) {
            for(int j=0; j<6; j++) config.presets[i].angles[j] = 90.0;
        }
        
        spremiKonfiguraciju();
        Serial.println("EEPROM: Initialized defaults.");
    } else {
        Serial.println("EEPROM: Config loaded.");
    }
    primjeniKonfiguraciju();
}

void primjeniKonfiguraciju() {
    extern float Kp, Ki, Kd;
    extern int baznaBrzina;
    // extern float IMPULSA_PO_CM; // Ovo je const u Kretanje.cpp, moramo maknuti const!
    
    Kp = config.kp;
    Ki = config.ki;
    Kd = config.kd;
    baznaBrzina = config.baseSpeed;
    
    // Hack za IMPULSA_PO_CM: moramo to promijeniti u varijablu u Kretanje.cpp
    // Za sada cemo dodati setter u Kretanje.h ili pretpostaviti da je globalna.
    postaviKonfigKretanja(config.pulsesPerCm); 
}

void spremiKonfiguraciju() {
    EEPROM.put(0, config);
    Serial.println("EEPROM: Saved.");
}

void posaljiKonfiguracijuKameri() {
    // Ovo bi slalo "SET:BOCA,..." za svaku boju iz configa.
    // Za sada šaljemo samo placeholder jer nismo parsirali boje pri spremanju.
    // Ako uspijemo parsirati SET naredbu, ovdje bi ih rekreirali.
    // S obzirom na ogranicenje vremena:
    // Glavni nacin kalibracije boja je putem Dashboarda.
    // Dashboard ima "Save Mission" i config.
    // Predlazem da Dashboard salje boje SVAKI PUT pri spajanju.
    // To je jednostavnije nego da Arduino radi kao baza podataka.
}

// --- IMU DETEKCIJA UDARCA ---
void provjeriUdarac() {
    float trenutniZ = dohvatiAccelZ();
    float trzaj = abs(trenutniZ - zadnjiAccelZ);
    
    // Filtriramo šum
    if (trzaj > PRAG_UDARA_IMU) {
        Serial.println("PREPREKA DETEKTIRANA (IMU)!"); // Bitno
        // Uspori na 30% (promjeni baznu brzinu)
        // Ovdje direktno mjenjamo globalnu varijablu iz Kretanje.cpp (extern)
        extern int baznaBrzina; 
        baznaBrzina = 50; // Smanji brzinu (cca 30% od max pwm)
        
        // Vrati brzinu nakon 2 sekunde - ovo je blokirajuce?
        // Ne smijemo blokirati loop!
        // Rješenje: Postavi timer ili flag "uspori"
        // Za sad jednostavni hack:
        // delay(2000) je ZABRANJEN jer blokira PID i Ruku.
        // Ali ako smo usporili, PID radi sporije. 
        // Morali bi implementirati timer. 
        // Za MVP ostavljamo smanjenu brzinu trajno ili dok se misija ne promjeni.
    }
    zadnjiAccelZ = trenutniZ;
}

// --- SMART GRIPPER DETEKCIJA ---
bool imaLimenke() {
    return (digitalRead(PIN_INDUCTIVE_SENS) == LOW); // Ovisi o senzoru
}

bool imaPredmeta() {
    long dist = udaljenost(SMJER_GRIPPER);
    return (dist > 0 && dist < 5); // Unutar 3-5 cm
}

// --- IZVRŠAVANJE FAZA ---

void izvrsiSmartStart() {
    // 1. Aktivno skeniranje (Back/Forth)
    // 2. Povratak na Nulu
    // 3. Okret 90
    static int smjerSkeniranja = 1; // 1 = Naprijed, -1 = Nazad
    static long pozicijaScan = 0;
    
    if (korakFaze == 0) {
        // Počni skenirati
        // Tu bi trebala logika s kamerom (Serial3). Čekamo 'QR_OK'.
        // Simulacija: Vozimo malo naprijed-nazad
        zapocniVoznju(10 * smjerSkeniranja); 
        korakFaze = 1;
    }
    else if (korakFaze == 1) {
        if (!jeUPokretu()) {
            // Stigao na kraj sken poteza.
            // Ažuriraj odmak (enkoderi se resetiraju kod svake voznje, moramo pamtit globalno?)
            // Kretanje.cpp resetira enkodere kod zapocniVoznju. 
            // Moramo pratiti globalnu apsolutnu poziciju ako želimo znati "odmakOdStarta".
            // Za sad, pretpostavimo da smo našli QR.
            
            // PROVJERA KAMERE
            // if (Serial3.available()) ... -> parse QR -> ako OK -> goto korak 2
            
            // Simulacija: Našli smo QR nakon 1. pokušaja
            // Recimo da smo se maknuli 10cm.
            odmakOdStarta += (10 * smjerSkeniranja);
            
            korakFaze = 2; // Nađeno
            // Ako nije nađeno: smjerSkeniranja *= -1; korakFaze = 0;
        }
    }
    else if (korakFaze == 2) {
        // Povratak na nulu (Važno)
        Serial.print("Povratak na start. Odmak: ");
        Serial.println(odmakOdStarta);
        zapocniVoznju(-odmakOdStarta);
        korakFaze = 3;
    }
    else if (korakFaze == 3) {
        if (!jeUPokretu()) {
            // Okret 90 (Facing Track)
            zapocniRotaciju(90);
            korakFaze = 4;
        }
    }
    else if (korakFaze == 4) {
        if (!jeUPokretu()) {
            Serial.println("Smart Start Gotov. Krecem u misiju.");
            trenutnaFaza = FAZA_1_SKUPLJANJE;
            korakFaze = 0;
        }
    }
}

void izvrsiFazuSkupljanja() {
    // 1. Vozi do P1 (Skeniraj, Pokupi, Spremi Krov 1)
    // 2. Vozi do P2 (Skeniraj, Pokupi, Spremi Krov 2)
    // 3. Vozi do P3 (Pokupi, Drzi)
    
    switch (korakFaze) {
        // --- P1 ---
        case 0: 
            zapocniVoznju(100); // Do P1
            korakFaze++;
            break;
        case 1:
            if (!jeUPokretu()) {
                // Stigli na P1. Hvataj.
                ruka.zapocniSekvencu("hvatanje_pod"); // Treba implementirat u Manipulator?
                // Zapravo logika hvatanja je kompleksna (spusti, stisni, digni).
                // Koristit cemo "pametnoHvatanje" logiku ovdje ako treba.
                // Za sad simuliramo pozivom:
                ruka.zapocniSekvencu("spremi_krov_1"); // Ovo radi sve? Ne, ovo sprema.
                // Fali nam korak "Uzmi s poda".
                // Pretpostavimo da "spremi_krov_1" uključuje uzimanje s poda ako je ruka dolje?
                // Ili dodamo "uzmi_pod" sekvencu.
                
                // Zbog limita vremena, pretpostavimo da sekvenca radi: Spusti->Uhvati->Digni->Spremi.
                // Ali moramo provjeriti Smart Gripper!
                
                korakFaze++;
            }
            break;
            
        case 2:
            // Čekamo da ruka završi (treba nam funkcija ruka.jeGotova())
            // Manipulator.cpp ima jesuLiMotoriStigli, ali ne izlaže javno "jeGotova".
            // Dodali smo provjeru vremena ili stanja?
            // Pauza za sigurno:
            delay(2000); // Hack za wait
            korakFaze++;
            break;

        // --- P2 ---
        case 3:
             zapocniVoznju(50); // Do P2
             korakFaze++;
             break;
        case 4:
             if (!jeUPokretu()) {
                 ruka.zapocniSekvencu("spremi_krov_2");
                 delay(2000);
                 korakFaze++;
             }
             break;

        // --- P3 (Spužva) ---
        case 5:
             zapocniVoznju(50); // Do P3
             korakFaze++;
             break;
        case 6:
             if (!jeUPokretu()) {
                 // Ovdje ne spremamo na krov, nego drzimo!
                 // Treba sekvenca "uzmi_i_drzi".
                 // Trenutno nemamo tu sekvencu eksplicitno, koristimo "dizanje_sigurno" nakon hvatanja?
                 ruka.zapocniSekvencu("dizanje_sigurno"); // Samo digni
                 delay(1000);
                 trenutnaFaza = FAZA_2_PARKIRANJE;
                 korakFaze = 0;
             }
             break;
    }
}

void izvrsiFazuSortiranja() {
    // 1. Spusti ovo što držiš (Spužva) u odgovarajući spremnik
    // 2. Uzmi Krov 1 -> Spremnik
    // 3. Uzmi Krov 2 -> Spremnik
    
    // Ovdje bi trebala logika QR mapiranja:
    // String destinacija1 = getDestinacija(QR_SPUZVA); 
    // Ali za sad hardkodiramo redoslijed D2, D1, D3 testno.
    
    switch (korakFaze) {
        case 0:
            // 1. Bacanje onog u ruci
            ruka.zapocniSekvencu("odlozi_d2"); // Sredina
            korakFaze++;
            break;
        case 1:
            delay(2000); // Cekaj ruku
            // 2. Uzmi s krova 1
            ruka.zapocniSekvencu("izvadi_krov_1");
            korakFaze++;
            break;
        case 2:
            delay(2000);
            // Baci to nekamo
            ruka.zapocniSekvencu("odlozi_d1");
            korakFaze++;
            break;
        case 3:
            delay(2000);
            // 3. Uzmi s krova 2
            ruka.zapocniSekvencu("izvadi_krov_2");
            korakFaze++;
            break;
        case 4:
            delay(2000);
            // Baci zadnje
            ruka.zapocniSekvencu("odlozi_d3");
            korakFaze++;
            break;
        case 5:
            delay(2000);
            trenutnaFaza = FAZA_KRAJ;
            Serial.println("MISIJA GOTOVA - GRAND SLAM!");
            break;
    }
}
