# Dokumentacija Protokola i Kretanja Robota

Ovaj dokument opisuje komunikacijski protokol između Robota i Dashboard aplikacije, strukturu glavne petlje (`loop`) i implementirane funkcije kretanja.

## 1. Komunikacijski Protokol (JSON)

Robot komunicira s Dashboard-om putem Bluetooth-a (Serial2) koristeći JSON format poruka.

### Naredbe (Dashboard -> Robot)

| Komanda | JSON Format | Opis | Funkcija |
| :--- | :--- | :--- | :--- |
| **Vozi Ravno** | `{"cmd": "straight", "val": <cm>}` | Vozi pravocrtno zadanu udaljenost u cm. | `voziRavno(val)` |
| **Vozi Manualno** | `{"cmd": "move_dual", "l": <pwm>, "r": <pwm>}` | Pokreće motore zadanom brzinom (-255 do 255). Ne staje sama! | `vozi(l, r)` |
| **Stani** | `{"cmd": "stop"}` | Odmah zaustavlja oba motora. | `stani()` |
| **Okreni** | `{"cmd": "turn", "val": <stupnjevi>}` | Okreće robota na mjestu (Point Turn) za zadani kut. | `okreni(val)` |
| **Skreni** | `{"cmd": "pivot", "val": <stupnjevi>}` | Skreće oko jednog kotača (Pivot Turn) za zadani kut. | `skreni(val)` |

### Odgovori (Robot -> Dashboard)

Nakon izvršene (ili započete) radnje, Robot šalje JSON odgovor:

*   `{"status": "DONE"}` - Poslano nakon što blokirajuća radnja završi (`straight`, `turn`, `pivot`).
*   `{"status": "OK"}` - Poslano odmah nakon pokretanja ne-blokirajuće radnje (`move_dual`).
*   `{"status": "STOPPED"}` - Poslano nakon naredbe `stop`.

---

## 2. Arduino Loop Funkcija

Glavna `loop()` funkcija u `Robot_Main.ino` je dizajnirana da bude minimalistička i reaktivna.

1.  **Čekanje Poruke**: Funkcija konstantno provjerava `Serial2` (Bluetooth) buffer.
2.  **Parsiranje**: Kada stigne poruka (završena s `\n`), parsira se kao JSON.
3.  **Usmjeravanje (Routing)**: Ovisno o polju `cmd`, poziva se odgovarajuća funkcija iz biblioteke `Kretanje`.
4.  **Blokiranje**: Naredbe `straight`, `turn` i `pivot` su **blokirajuće**. To znači da `loop` neće procesuirati nove poruke dok se radnja ne izvrši do kraja. Naredba `move_dual` je **ne-blokirajuća** - motori se pokrenu i `loop` nastavlja slušati (npr. za `stop`).

---

## 3. Implementirane Funkcije Kretanja (`Kretanje.cpp`)

Sve funkcije nalaze se u biblioteci `Kretanje`.

### `void voziRavno(float cm)`
*   **Logika**: Resetira enkodere i vozi dok prosjek oba enkodera ne dosegne preračunati broj pulseva za `cm`.
*   **Sinkronizacija**: Koristi P-regulator na razlici enkodera (`L - R`). Ako lijevi motor ide brže (veći put), usporava ga i ubrzava desni kako bi održao pravac.
*   **Senzori**: Koristi **samo enkodere**. Žiroskop se ne koristi.
*   **Tip**: Blokirajuća.

### `void vozi(int lijeviMotor, int desniMotor)`
*   **Logika**: Direktno postavlja PWM vrijednosti na motore.
*   **Parametri**: Vrijednosti od -255 (maksimalno nazad) do 255 (maksimalno naprijed). 0 je stop.
*   **Senzori**: Nema povratne veze (open-loop).
*   **Tip**: Ne-blokirajuća. Robot nastavlja voziti dok se ne pozove `stani()` ili nova `vozi()` naredba.

### `void okreni(float kut)`
*   **Logika**: Point Turn (okret na mjestu). Kotači se vrte u suprotnim smjerovima.
*   **Smjer**: Pozitivan kut (> 0) okreće udesno (CW). Negativan kut (< 0) okreće ulijevo (CCW).
*   **Senzori**: Koristi **IMU (Žiroskop)**. Integrira kutnu brzinu Z-osi da dobije trenutni odmak kuta. Staje kada dosegne ciljni kut.
*   **Tip**: Blokirajuća.

### `void skreni(float kut)`
*   **Logika**: Pivot Turn (okret oko jednog kotača). Jedan kotač stoji, drugi vozi.
*   **Smjer**:
    *   Kut > 0 (Desno): Lijevi motor vozi, desni stoji.
    *   Kut < 0 (Lijevo): Desni motor vozi, lijevi stoji.
*   **Senzori**: Koristi **IMU (Žiroskop)** kao i `okreni`.
*   **Tip**: Blokirajuća.

### `void stani()`
*   **Logika**: Postavlja PWM oba motora na 0 i gasi pinove smjera (kočenje/coast ovisno o driveru).
