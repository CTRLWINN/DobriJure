# 🛠 Dokumentacija Hardvera (HardwareMap)

Ovaj dokument opisuje fizički sloj robota, pinove i način povezivanja komponenti. Sve definicije nalaze se u `HardwareMap.h`.

## 1. Pogon (Diferencijalni)
Koristimo **L298N** ili sličan H-Most driver za upravljanje dvama DC motorima.

| Funkcija | Pin (Arduino) | Opis |
| :--- | :--- | :--- |
| **Lijevi PWM** | 12 | Brzina lijevog motora (0-255) |
| **Lijevi IN1** | 8 | Smjer A |
| **Lijevi IN2** | 7 | Smjer B |
| **Desni PWM** | 11 | Brzina desnog motora (0-255) |
| **Desni IN1** | 6 | Smjer A |
| **Desni IN2** | 5 | Smjer B |

## 2. Senzori
Senzori omogućuju robotu percepciju okoline.

### A. Senzori Linije (IR Array)
5-kanalni infracrveni senzor spojen na analogne ulaze.
*   **Pinovi:** A0 - A4
*   **Svrha:** Praćenje crne linije na bijeloj podlozi (PID regulacija).

### B. Lane Assist (Bočni IR)
Dva dodatna IR senzora na prednjim bumperima.
*   **Lijevi:** Pin 48
*   **Desni:** Pin 49
*   **Svrha:** Sprječavanje izlaska sa staze (sigurnosni override).

### C. Smart Gripper (Hvataljka)
Hvataljka je opremljena senzorima za detekciju predmeta.
*   **Induktivni Senzor:** Pin 4 (Detektira metal/limenke).
*   **Nicla Vision TOF:** Integrirani senzor udaljenosti na kameri. **Neophodan** za detekciju predmeta u hvataljci (< 5cm preciznost).

### D. IMU (Inercijska Mjerna Jedinica)
**LSM9DS1** senzor spojen putem I2C sabirnice.
*   **Adresa:** 0x1E (Magnetometar), 0x6B (Accel/Gyro).
*   **Svrha:** 
    1.  **Žiroskop/Kompas:** Održavanje ravnog smjera i precizni okreti za 90°.
    2.  **Akcelerometar:** Detekcija udarca o prepreku (olovku) putem nagle promjene G-sile (Jerk).

## 3. Manipulator (Robotska Ruka)
Upravljan putem **PCA9685** 16-kanalnog PWM drivera (I2C Adresa: 0x40).

| Kanal | Zglob | Opis |
| :--- | :--- | :--- |
| 0 | Baza | Rotacija lijevo-desno (0-180°) |
| 1 | Rame | Dizanje/spuštanje donjeg dijela (0-180°) |
| 2 | Lakat | Dizanje/spuštanje gornjeg dijela (0-180°) |
| 3 | Zglob | Rotacija zgloba (0-180°) |
| 4 | Rot | Rotacija šake (0-180°) |
| 5 | Hvat | Otvaranje/zatvaranje (0-180°) |

## 4. Komunikacija
*   **Serial2 (16/17):** Bluetooth modul (HC-02) za vezu s PC Dashboardom.
*   **Serial3 (14/15):** Nicla Vision kamera za AI obradu slike.

---
**Napomena:** Uvijek provjerite fizičke spojeve prije uključivanja napajanja!
