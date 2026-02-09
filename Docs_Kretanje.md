# 🏎️ Dokumentacija Kretanja i Senzora

Ovaj modul (`Kretanje.h` / `Kretanje.cpp`) zadužen je za preciznu navigaciju robota po stazi.

## 1. Diferencijalni Pogon i PID Kontrola
Robot koristi diferencijalni pogon (dva neovisna kotača). Za ravno kretanje i praćenje linije koristi se **PID Regulator** (Proportional-Integral-Derivative).

### PID Objašnjenje
*   **P (Proporcionalno):** Reagira na trenutnu grešku (udaljenost od linije). Što je greška veća, korekcija je jača.
*   **I (Integralno):** Zbraja greške kroz vrijeme. Pomaže ako robot stalno malo "vuče" u jednu stranu.
*   **D (Derivacijsko):** Predviđa buduću grešku prateći brzinu promjene. Sprječava nagle oscilacije (cikanje-cakanje).
*   **Formula:** `Izlaz = (Kp * Greška) + (Ki * Integral) + (Kd * Derivacija)`

## 2. Lane Assist (Sigurnosni Sustav)
Sustav aktivno sprječava izlijetanje sa staze koristeći dodatne IR senzore na bumperima.
*   **Logika:** Ovo je "Hard Override". Ako lijevi senzor vidi rub staze, robot **odmah** skreće desno, ignorirajući PID.
*   Ovo je ključno za brzu vožnju gdje PID možda ne stigne reagirati na oštar zavoj.

> [!WARNING]
> **Bitno:** Navigacija ovisi o stabilnosti ruke. Ako manipulator nije u `POZA_VOZNJA`, podaci s kamere neće biti validni.

## 3. Odometrija i Pozicioniranje
Koristimo enkodere na motorima za mjerenje prijeđenog puta.
*   `IMPULSA_PO_CM`: Konstanta koja definira koliko impulsa enkodera odgovara 1 cm puta.
*   **Smart Start:** Robot koristi odometriju (`odmakOdStarta`) da zapamti koliko je vozio tijekom skeniranja QR koda i precizno se vrati na startnu liniju.

## 4. IMU (Detekcija Prepreka)
Akcelerometar (LSM9DS1) se koristi za detekciju fizičkog kontakta s preprekom (olovkom).
*   Mjerimo **Jerk** (trzaj) po Z-osi: `abs(trenutni_acc_z - zadnji_acc_z)`.
*   Ako trzaj pređe prag (`PRAG_UDARA_IMU`), robot usporava na 30% brzine radi sigurnog prelaska.

## 5. Ultrazvučni Senzori
Smart Gripper koristi ultrazvučni senzor za potvrdu hvatanja.
*   `udaljenost(SMJER_GRIPPER)`: Mjeri ima li nečega unutar hvataljke.
*   Ako je udaljenost < 5cm, smatramo da je predmet uspješno uhvaćen.
