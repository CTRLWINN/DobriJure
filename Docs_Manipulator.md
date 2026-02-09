# 🤖 Dokumentacija Manipulatora (Robotska Ruka)

Manipulator je 6-DOF (Degree of Freedom) robotska ruka koja se koristi za hvatanje, podizanje i sortiranje predmeta (boce, limenke, spužve). Logika je implementirana u klasi `Manipulator`.

## 1. Kinematika i Upravljanje
Sustav ne koristi potpunu inverznu kinematiku (IK) zbog složenosti i ograničenja procesora, već se oslanja na **Forward Kinematics** i unaprijed definirane kutove za ključne pozicije.

### Soft-Start Tehnologija
Kako bi se izbjeglo naglo trzanje i preopterećenje servo motora, implementiran je "Soft-Start" algoritam.
*   Umjesto trenutnog postavljanja kuta (npr. s 0° na 90°), ruka se pomiče u malim koracima (`KORAK_MK` = 1.0 stupanj po ciklusu).
*   Ovo osigurava fluidno kretanje i smanjuje strujne udare.

## 2. Ključne Pozicije (Kutovi)
Definirane su u `Manipulator.h`:

| Pozicija | Kut Baze | Opis |
| :--- | :--- | :--- |
| **Krov Slot 1** | 10° | Skladište na lijevoj strani krova. |
| **Krov Slot 2** | 170° | Skladište na desnoj strani krova. |
| **Odlaganje D1** | 45° | Lijevi spremnik (Limenka/Boca/Spužva). |
| **Odlaganje D2** | 90° | Srednji spremnik. |
| **Odlaganje D3** | 135° | Desni spremnik. |
| **POZA_VOZNJA** | 90° | Ruka podignuta, lakat na 90°, kamera gleda ispred robota. Ova pozicija je **presudna** za stabilnu navigaciju. |

## 3. State Machine (Stanja Ruke)
Ruka radi kao konačni automat (FSM), što omogućuje asinkrono izvršavanje bez blokiranja glavne petlje.

1.  **STANJE_MIRUJE:** Ruka čeka komandu.
2.  **STANJE_DIZANJE_SIGURNO:** Podiže se u sigurnu visinu prije rotacije kako ne bi udarila u tijelo robota.
3.  **STANJE_ROTACIJA_BAZE:** Okreće se prema cilju (D1/D2/D3 ili Krov).
4.  **STANJE_SPUSTANJE:** Spušta se na visinu za ispuštanje.
5.  **STANJE_ISPUSTANJE:** Otvara hvataljku.
6.  **STANJE_POVRATAK:** Vraća se u neutralni (Home) položaj.
7.  **STANJE_HVAITANJE_S_KROVA:** Posebna sekvenca za uzimanje predmeta spremljenih na krovu.

## 4. Korištenje u Kodu
```cpp
// Inicijalizacija
Manipulator ruka;

// Pokretanje sekvence
ruka.zapocniSekvencu("spremi_krov_1");

// Ažuriranje (mora biti u loop petlji)
void loop() {
    ruka.azuriraj();
}
```
Detalji implementacije nalaze se u `Manipulator.cpp`.
