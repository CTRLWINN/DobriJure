# 🎮 Mission Control Dashboard (dashboard_v2.py)

Ovaj dokument opisuje funkcionalnosti glavne PC aplikacije za upravljanje robotom. Aplikacija služi za telemetriju, ručno upravljanje, učenje misije i autonomno izvršavanje.

## Instalacija i Pokretanje

1.  **Python Okruženje**:
    *   Potreban Python 3.10 ili noviji.
    *   Instalirati biblioteke: `pip install customtkinter pyserial requests Pillow`
2.  **Pokretanje**:
    *   Pokreni skriptu: `python dashboard_v2.py`
    *   Aplikacija će se otvoriti u "Dark" modu.

---

## 🔌 Povezivanje

Prije korištenja bilo kojeg taba, potrebno je uspostaviti vezu s robotom.

1.  **Bluetooth/Serial**: Odaberi COM port robota (HC-02) iz padajućeg izbornika u lijevom stupcu i klikni **"Connect BLE/UART"**.
2.  **Nicla Vision IP**: Unesi IP adresu kamere (zadan: `192.168.0.7`) za video stream.

---

## Tab 1: Kalibracija

Ovaj tab služi za fino podešavanje parametara robota bez potrebe za recompajliranjem koda.

> [!TIP]
> ![Tab Kalibracija](slike/tabKalibracija.png)
> *(Screenshot taba Kalibracija)*

### Funkcionalnosti:
*   **PID Config**: Unos Kp, Ki, Kd konstanti za regulaciju smjera.
*   **Motor Config**: Postavljanje `Pulse/cm` (koliko pulseva enkodera je 1 cm) i osnovne brzine.
*   **Video Stream**: Prikaz slike s Nicla Vision kamere (start stream gumb).
*   **Kalibracija Boja (LAB)**:
    *   Moguće odabrati objekt (BOCA, LIMENKA, SPUZVA).
    *   Podesiti LAB min/max granice.
    *   **"Pošalji Boje (SET)"**: Šalje nove parametre kameri u realnom vremenu.

---

## Tab 2: Učenje (Manual)

Glavni tab za ručnu vožnju i snimanje rute ("Teach & Repeat").

> [!TIP]
> ![Tab Učenje](slike/tabManualno.png)
> *(Screenshot taba Učenje)*

### Kontrole:
*   **Tipkovnica (Numpad)**:
    *   `8` (Naprijed), `2` (Natrag), `4` (Lijevo), `6` (Desno).
    *   `SPACE`: Spremi trenutnu poziciju (Waypoint) u listu.
*   **Manipulator (Ruka)**:
    *   Odaberi preset (npr. "Uzmi_Boca") i klikni **"Izvrši & Snimi Ruku"**.
    *   Robot će izvršiti pokret i dodati to kao korak u misiju.
*   **Telemetrija**: Prikaz udaljenosti (cm), enkodera, stanja senzora i ruke u stvarnom vremenu.
*   **Snimanje Misije**:
    *   Svi potezi se bilježe u desnom prozoru.
    *   Klik na **"Spremi Misiju (misija.txt)"** generira datoteku koju autonomni mod koristi.

---

## Tab 3: Autonomno

Izvršavanje prethodno snimljene misije.

> [!TIP]
> ![Tab Autonomno](slike/tabAutonomno.png)
> *(Screenshot taba Autonomno)*

### Proces:
1.  **Učitaj Misiju**: Učitava `misija.txt` u prozor za pregled.
2.  **POKRENI MISIJU**:
    *   Dashboard šalje `MODE:AUTO`.
    *   Zatim liniju po liniju šalje naredbe robotu (npr. `MOVE:150`, `ARM:Uzmi_Boca`).
    *   **Čeka izvršenje**: Prati telemetriju robota dok ne potvrdi da je naredba izvršena (npr. dok se distanca ne promijeni za zadani iznos).
3.  **STOP**: Hitno zaustavljanje robota.

---

## Rješavanje Problema

*   **Robot ne odgovara**: Provjeri je li COM port točan i je li baterija spojena (L298N logika).
*   **Video stream kasni**: Smanji rezoluciju ili provjeri WiFi signal.
*   **"Unknown Preset"**: Provjeri jesu li imena preseta u Dashboardu ista kao u `Robot_Main.ino`.
