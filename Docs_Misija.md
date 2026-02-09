# 🎯 Dokumentacija Misije ("Grand Slam")

Ova dokumentacija opisuje logiku izvršavanja glavne misije robota, implementiranu u `Robot_Main.ino`.

## 1. Strategija "Grand Slam"
Cilj je sakupiti sve predmete (Boca, Limenka, Spužva) u jednom prolazu bez višestrukog vraćanja na start.

### Faze Misije
Misija je podijeljena u 3 glavne faze:

1.  **FAZA 1: Skupljanje (Harvest)**
    *   Robot vozi do pozicija P1 i P2.
    *   Sakupljene predmete (npr. krovne elemente) sprema na **Krov** (Slot 1 i Slot 2).
    *   Vozi do P3 (Spužva), uzima je i drži u hvataljci.
    
2.  **FAZA 2: Parkiranje**
    *   Robot s punim teretom vozi do središnjeg spremnika (D2).
    *   Koristi se precizna navigacija i Lane Assist za sigurnost.

3.  **FAZA 3: Sortiranje (Stationary Disposal)**
    *   Robot stoji ispred D2.
    *   **Korak A:** Baca predmet iz hvataljke (Spužva) u odgovarajući spremnik (D1/D2/D3).
    *   **Korak B:** Uzima predmet s Krov Slot 1 i baca ga u odgovarajući spremnik.
    *   **Korak C:** Uzima predmet s Krov Slot 2 i baca ga u odgovarajući spremnik.

## 2. Smart Start (Pametni Start)
Kako bi se osigurala maksimalna preciznost, robot na početku izvodi kalibracijsku sekvencu:
1.  **Skeniranje:** Robot vozi naprijed-nazad tražeći QR kod kamerom.
2.  **Odometrija:** Pamti točan odmak (`odmakOdStarta`) od fizičke startne linije.
3.  **Povratak:** Vraća se točno na nulu kako bi poništio grešku pozicije.
4.  **Rotacija:** Okreće se za 90° prema stazi koristeći IMU (kompas).

## 3. Dijagram Toka (State Machine)
```text
[CEKANJE_STARTA] --> (Start Komanda) --> [SMART_START]
                                              |
                                              v
                                      [FAZA_1_SKUPLJANJE]
                                      (P1 -> Krov1, P2 -> Krov2, P3 -> Ruka)
                                              |
                                              v
                                      [FAZA_2_PARKIRANJE]
                                      (Vozi do D2)
                                              |
                                              v
                                      [FAZA_3_SORTIRANJE]
                                      (Ruka -> Dx, Krov1 -> Dy, Krov2 -> Dz)
                                              |
                                              v
                                         [KRAJ]
```

## 4. Pokretanje Misije
Misija se može pokrenuti na dva načina:
1.  **Ble/Serial Monitor:** Slanjem bilo kojeg znaka (ili "MODE:AUTO").
2.  **Dashboard:**
    *   Učitati misiju iz `misija.txt` u tabu "Autonomno".
    *   Pritisnuti "POKRENI MISIJU".
    *   Dashboard šalje `MODE:AUTO` i zatim iterira kroz listu naredbi (MOVE, ARM, WAIT) šaljući ih robotu jednu po jednu.
