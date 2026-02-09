# **🤖 WSC Autonomni Robot (Full-Stack Sustav)**

**Napredni modularni robotski sustav s AI vizijom, 6-DOF Manipulatorom i "Mission Control" Dashboardom.**

Ovaj repozitorij sadrži izvorni kod, dokumentaciju i alate za autonomnog robota razvijenog za natjecanje **WorldSkills Croatia 2026** u disciplini Robotika. Sustav integrira ugrađene sustave na Arduino platformi, AI obradu slike na rubu (Edge AI) i napredno PC sučelje za upravljanje misijama.

## **📋 Sadržaj**

- [Značajke Sustava](#-značajke-sustava)
- [Hardverska Mapa](#-hardverska-mapa)
- [Softverska Arhitektura](#-softverska-arhitektura)
- [Komunikacijski Protokol](#-komunikacijski-protokol)
- [Instalacija i Pokretanje](#-instalacija-i-pokretanje)
- [Autori](#-autori)
- [Licenca](#-licenca)

## **🌟 Značajke Sustava**

*   **Napredna Kinematika:**
    *   **PID Navigacija:** Precizno praćenje linije i žiro-stabilizirana rotacija.
    *   **6-DOF Manipulator:** Robotska ruka u **Eye-in-Hand** konfiguraciji (Nicla Vision na gripperu) za preciznu manipulaciju i inspekciju. "Soft-Start" tehnologija za glatke pokrete.
*   **Mission Control Dashboard:**
    *   Python aplikacija (`dashboard_v2.py`) s BLE komunikacijom.
    *   **[Pročitaj Detaljne Upute (MissionControl.md)](MissionControl.md)** - Opis svih tabova i funkcionalnosti.
    *   Moduli: Nadzor, Podešavanje, Planer Misije, Učenje.
    *   **"Teach-in" Mod:** Ručno vođenje robota i snimanje ključnih točaka (Keyframes) za autonomnu reprodukciju.
*   **IoT & Telemetrija:**
    *   Dvosmjerna JSON komunikacija putem Bluetootha (BLE/Classic).
    *   Real-time prikaz enkodera, brzine, orijentacije i statusa baterije.
*   **AI Vizija:**
    *   Nicla Vision modul za detekciju QR kodova i klasifikaciju otpada putem Neural Networka.

## **🛠 Hardverska Mapa**

Glavni kontroler: **Arduino Mega 2560**.

| Podsustav | Komponenta | Pinovi / Protokol | Opis |
| :--- | :--- | :--- | :--- |
| **Pogon** | L298N + DC Motori | 11, 12 (PWM) | Diferencijalni pogon. |
| **Enkoderi** | Kvadraturni | 18, 19 (L) / 2, 3 (R) | Interrupt-driven odometrija. |
| **Manipulator** | PCA9685 Driver | I2C (0x40) | Upravlja sa 7 servo motora. |
| **IMU** | LSM9DS1 | I2C (0x1E) | 9-osni žiroskop/akcelerometar. |
| **Komunikacija** | HC-02 Bluetooth | Serial2 (16/17) | Veza s PC Dashboardom. |
| **Vizija** | Nicla Vision | Serial3 (14/15) | UART veza za AI rezultate. |
| **Linija** | 5-ch IR Array | A0 - A4 | Analogno čitanje pozicije. |

### Mapiranje Kanala Ruke (PCA9685)
*   **0:** Baza (Waist)
*   **1:** Rame (Shoulder)
*   **2:** Lakat (Elbow)
*   **6:** Hvataljka (Gripper)
*   *(Vidi `Upute_Ruka.md` za detalje)*

## **📂 Softverska Arhitektura**

Projekt je organiziran u modularne cjeline:

### 1. Firmware (`Robot_Main.ino`)
Glavna petlja koja orkestrira podsustave.
*   `Kretanje.h`: Upravljanje motorima i PID regulatorom.
*   `Manipulator.h`: Logika robotske ruke i State Machine za animacije.
*   `HardwareMap.h`: Centralizirana definicija hardvera.
*   `Enkoderi.h` / `IMU.h`: Drajveri za senzore.

### 2. PC Dashboard (`dashboard_v2.py`)
Moderno GUI sučelje napisano u Pythonu (CustomTkinter) s **BLE** podrškom.
*   **[MissionControl.md](MissionControl.md)** sadrži detaljan opis sučelja.
*   **Tab 1 (Nadzor):** Kompas, kamera uživo, telemetrija.
*   **Tab 2 (Podešavanje):** Live tuning PID varijabli i kalibracija senzora.
*   **Tab 3 (Misija):** Sekvencer za slaganje autonomnih zadataka (blokovsko programiranje).
*   **Tab 4 (Učenje):** Snimanje putanje vožnjom robota (WASD).

## **📡 Komunikacijski Protokol**

Sustav koristi robustan **JSON protokol** preko UART/BLE veze.
Vidi **[Upute_Protocol_JSON.md](Upute_Protocol_JSON.md)** za potpunu specifikaciju.

**Primjer:**
*   PC -> Robot: `{"cmd": "manual", "l": 100, "r": 100}`
*   Robot -> PC: `{"enc_l": 1200, "spd": 0.5, "head": 90.0}`

## **🚀 Instalacija i Pokretanje**

### 1. Arduino
1.  Otvorite `Robot_Main.ino`.
2.  Instalirajte biblioteke: `Adafruit PWMServoDriver`, `ArduinoJson`, `SparkFun LSM9DS1`.
3.  Uploadajte na Arduino Mega.

### 2. Python Dashboard
1.  Instalirajte ovisnosti:
    ```bash
    pip install customtkinter pyserial bleak opencv-python pillow
    ```
2.  Pokrenite aplikaciju:
    ```bash
    python dashboard.py
    ```

## **⚙️ Posebne Upute**
*   **Kalibracija Ruke:** Pogledaj `Upute_Ruka.md` prije prvog pokretanja kako bi se izbjegla fizička oštećenja.
*   **BLE Povezivanje:** Dashboard koristi `Bleak` za automatsko skeniranje uređaja s imenom "HC-02".

## **👥 Autori**

**Tim:** **[Ctrl+Win]**  
**Škola:** **[Gimnazija i strukovna škola Jurja Dobrile Pazin]**

*   **[Noel Pujas]** - *Lead Programmer & AI*
*   **[Matteo Mladossich]** - *Mechanical Design & Electronics*
*   **[Lovro Šverko, prof.]** - *Mentor*

## **� Dokumentacija Modula**
Detaljna objašnjenja za edukaciju i razvoj:

*   **[Hardver i Pinovi (Docs_Hardver.md)](Docs_Hardver.md)** - Popis svih senzora, motora i spojeva.
*   **[Manipulator i Kinematika (Docs_Manipulator.md)](Docs_Manipulator.md)** - Kako radi ruka, kutovi i sekvence.
*   **[Mission Control (MissionControl.md)](MissionControl.md)** - Upute za korištenje Dashboard aplikacije.
*   **[Kretanje i Senzori (Docs_Kretanje.md)](Docs_Kretanje.md)** - PID kontrola, Lane Assist i IMU detekcija prepreka.
*   **[Misija i Strategija (Docs_Misija.md)](Docs_Misija.md)** - Objašnjenje "Grand Slam" strategije i Smart Start sekvence.

## **🚀 Novo u Verziji 2026 (Grand Slam)**
*   **Smart Start:** Automatsko pozicioniranje prije utrke.
*   **Lane Assist:** Sigurnosni sustav protiv izlijetanja sa staze.
*   **Jerk Detection:** IMU detekcija udaraca o prepreke.
*   **Smart Gripper:** Provjera uspješnosti hvatanja ultrazvukom.

## **�📄 Licenca**
Ovaj projekt je otvorenog koda (Open Source) pod **MIT Licencom**.
Slobodno koristite i modificirajte kod za edukativne svrhe.
