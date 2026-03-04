# 📘 Projektna Dokumentacija: WSC Autonomni Robot

Ovaj dokument služi kao središnji priručnik za korištenje, razvoj i održavanje sustava **DobriJure** (WSC 2026). Objedinjuje sve tehničke aspekte robota, dashboard aplikacije i operativnih procedura.

---

## 📑 Sadržaj

1.  [Pregled Sustava](#1-pregled-sustava)
2.  [Brzi Početak (Quick Start)](#2-brzi-početak)
3.  [Korisnički Priručnik](#3-korisnički-priručnik)
4.  [Tehnička Referenca](#4-tehnička-referenca)
5.  [Rješavanje Problema](#5-rješavanje-problema)
6.  [Linkovi na Detaljne Module](#6-linkovi-na-detaljne-module)

---

## 1. Pregled Sustava

**DobriJure** je napredni mobilni robot dizajniran za natjecanje WorldSkills Croatia u disciplini Robotika. Sustav je "Full-Stack" rješenje koje se sastoji od:

*   **Robot (Firmware):** Arduino Mega 2560 koji upravlja motorima, senzorima i manipulatorom.
*   **Vision (Edge AI):** Nicla Vision kamera koja vrti TinyML model za detekciju otpada (Boca/Limenka/Spužva).
*   **Mission Control (Dashboard):** PC aplikacija za nadzor, telemetriju i vođenje misije.

### Ključne Mogućnosti
*   **Autonomna Vožnja:** PID praćenje linije s "Lane Assist" sustavom.
*   **Manipulator:** 6-osna ruka s pametnom hvataljkom (Smart Gripper).
*   **Strategija:** "Grand Slam" algoritam za optimizirano prikupljanje predmeta.
*   **Povezivost:** Bežična Bluetooth (BLE/Classic) komunikacija.

---

## 2. Brzi Početak

Ovdje su koraci za postavljanje razvojnog okruženja "od nule".

### A. Preduvjeti
*   **Računalo:** Windows 10/11.
*   **Softver:** 
    *   [Arduino IDE 2.0+](https://www.arduino.cc/en/software)
    *   [Python 3.10+](https://www.python.org/downloads/)
    *   [Git](https://git-scm.com/downloads)

### B. Priprema Robota (Firmware)
1.  Spoji Arduino Mega USB kabelom.
2.  Otvori `Robot_Main/Robot_Main.ino` u Arduino IDE.
3.  Instaliraj potrebne biblioteke (Tools -> Manage Libraries):
    *   `Adafruit PWMServoDriver`
    *   `ArduinoJson`
    *   `SparkFun LSM9DS1`
4.  Odaberi pločicu "Arduino Mega or Mega 2560" i odgovarajući COM port.
5.  Klikni **Upload**.

### C. Priprema Dashboarda
1.  Otvori terminal (CMD/PowerShell) u mapi `Dashboard`.
2.  Instaliraj Python biblioteke:
    ```powershell
    pip install customtkinter pyserial bleak opencv-python pillow requests
    ```
3.  Pokreni aplikaciju:
    ```powershell
    python dashboard_v2.py
    ```

---

## 3. Korisnički Priručnik

Kako koristiti robota u natjecateljskom okruženju.

### Povezivanje
1.  Upali robota (Prekidač na bateriji).
2.  U Dashboardu odaberi COM port (npr. `COM12` ili naziv Bluetooth uređaja `HC-02`).
3.  Klikni **"Connect"**. Status bi trebao postati "Connected".

### Kalibracija (Prije svake vožnje)
1.  Odi na tab **"Postavke"**.
2.  Provjeri sliku s kamere. Ako je pretamna/preosvijetljena, prilagodi rasvjetu prostorije.
3.  **IMU Kalibracija:** Postavi robota na ravno i klikni "Reset Gyro" da se nula poravna.

### Pokretanje Misije (Autonomno)
1.  Postavi robota na Startnu poziciju (unutar crnog okvira).
2.  U tabu **"Autonomno"** učitaj `misija.txt` (ili koristi zadanu).
3.  Klikni **"POKRENI MISIJU"**.
4.  **Smart Start:** Robot će prvo skenirati QR kod, a zatim krenuti u izvršavanje zadataka.

### Ručno Upravljanje
1.  Odi na tab **"Manual"**.
2.  Koristi **NumPad** tipke (8, 2, 4, 6) za vožnju.
3.  Koristi slidere za pomicanje svakog zgloba ruke zasebno.

---

## 4. Tehnička Referenca

Kratki pregled najvažnijih modula.

### Hardver
*   **Driver Motora:** L298N (Pinovi 11, 12 za PWM).
*   **Servo Driver:** PCA9685 (I2C 0x40). Napaja se zasebnim DC/DC pretvaračem.
*   **Senzori:** 5x IR Array (Linija), 2x IR (Bumperi), LSM9DS1 (IMU).

### Softver (Robot)
Arhitektura je bazirana na `non-blocking` kodu.
*   **`Robot_Main.ino`**: Glavna petlja (`loop`) poziva `azuriraj()` metode svih podsustava.
*   **`Kretanje.cpp`**: Implementira PID regulator.
*   **`Manipulator.cpp`**: Implementira State Machine za ruku.

### Komunikacija
Protokol je JSON baziran.
*   **Primjer Naredbe:** `{"cmd": "move", "dir": "fwd", "dist": 100}`
*   **Primjer Telemetrije:** `{"status": "idle", "batt": 7.4, "dist": 45}`

---

## 5. Rješavanje Problema

| Simptom | Mogući Uzrok | Rješenje |
| :--- | :--- | :--- |
| **Robot ne skreće** | PID parametri su preniski ili je brzina prevelika. | Povećaj `Kp` u Dashboardu ili smanji baznu brzinu. |
| **Ruka se trese** | Napon baterije je nizak ili je teret pretežak. | Napuni bateriju (LiPo 2S). Provjeri mehaniku. |
| **Kamera ne radi** | Nicla Vision se pregrijala ili nije na WiFi-u. | Resetiraj kameru. Provjeri IP adresu u Dashboardu. |
| **Bluetooth puca** | Prevelika udaljenost ili smetnje. | Približi laptop robotu (< 5m). Koristi vanjski BT dongle. |

---

## 6. Linkovi na Detaljne Module

Za dublje razumijevanje, prouči specifične dokumente:

*   📄 **[Hardver i Pinout (Docs_Hardver.md)](Docs_Hardver.md)**
*   📄 **[Upravljanje Kretanjem (PID) (Docs_Kretanje.md)](Docs_Kretanje.md)**
*   📄 **[Manipulator i Kinematika (Docs_Manipulator.md)](Docs_Manipulator.md)**
*   📄 **[Logika Misije (Docs_Misija.md)](Docs_Misija.md)**
*   📄 **[Protokol Komunikacije (Upute_Protocol_JSON.md)](Upute_Protocol_JSON.md)**
*   📄 **[Upute za Ruku (Upute_Ruka.md)](Upute_Ruka.md)**
*   📄 **[Dashboard Priručnik (MissionControl.md)](MissionControl.md)**

---
*Generirano od strane **Antigravity** asistenta, 2026.*
