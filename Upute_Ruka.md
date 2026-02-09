# Upute za Robotsku Ruku (Manipulator)

Ovaj dokument opisuje spajanje, logiku i kalibraciju 6-DOF robotske ruke za WorldSkills Croatia 2026.

## Tablica Spajanja (PCA9685)

Driver PCA9685 komunicira putem I2C protokola (Adresa `0x40`).
Spojite servo motore na sljedeće kanale:

| Kanal | Naziv (Varijabla)  | Dio Tijela       | Opis Funkcije                   |
|:-----:|:-------------------|:-----------------|:--------------------------------|
| **0** | `KANAL_BAZA`       | Baza (Waist)     | Rotacija cijele ruke (0-360°)   |
| **1** | `KANAL_RAME`       | Rame (Shoulder)  | Podizanje glavnog kraka         |
| **2** | `KANAL_LAKAT`      | Lakat (Elbow)    | Savijanje drugog zgloba         |
| **3** | `KANAL_PODLAKTICA` | Podlaktica       | Rotacija/pomak podlaktice       |
| **4** | `KANAL_ZGLOB_ROT`  | Zglob (Rotacija)| Rotacija šake                   |
| **5** | `KANAL_ZGLOB_NAGIB`| Zglob (Nagib)    | Nagib šake (Pitch)              |
| **6** | `KANAL_HVATALJKA`  | Hvataljka        | Otvaranje/zatvaranje            |

## Opis Logike

### Prijenosni Omjer Baze (2:1)
Baza ruke koristi **2:1 prijenosni omjer** zupčanika kako bi povećala doseg rotacije.
- Standardni servo se miče od 0° do 180°.
- Zbog prijenosa, ruka se fizički miče od 0° do 360°.
- **Kod:** Kada zatražite kut od `90°` na bazi, softver automatski preračunava potreban kut serva (45°) kako bi ruka fizički bila na 90°.

### Non-blocking "Soft-Start"
Kako bi kretanje bilo glatko i kako bi se spriječilo prevrtanje robota zbog naglih trzaja:
1. **Nikada ne koristimo `delay()`**: Procesor cijelo vrijeme vrti glavnu petlju (`loop()`).
2. **Inkrementalni pomak**: U funkciji `azuriraj()`, servo se pomiče samo za mali korak (`KORAK_MK`) prema cilju u svakom ciklusu.
3. **Automatsko Ažuriranje**: Glavna petlja mora pozivati `manipulator.azuriraj()` što češće.

## Kalibracija ("Zero" Pozicija)

Prije fizičkog spajanja plastičnih dijelova (rogova serva), važno je pronaći nultu ili središnju poziciju motora.

1. **Uploadajte kod** s inicijalizacijom manipulatora.
2. U konstruktoru `Manipulator::Manipulator()`, svi motori se postavljaju na **90°** (sredina).
3. Spojite napajanje i pričekajte da se motori postave u tu poziciju.
4. **Montirajte dijelove**:
   - **Baza**: Postavite ruku tako da gleda ravno naprijed (ili u sredinu raspona).
   - **Rame/Lakat**: Postavite ih pod pravim kutom ili u željenu "srednju" poziciju.
   - **Hvataljka**: Montirajte u poluzatvorenom položaju.
5. Nakon montaže, testirajte krajnje granice (`SERVO_MIN`, `SERVO_MAX`) oprezno kako ne biste polomili zupčanike.

## Korištenje
Komande se šalju putem JSON formata preko serijske veze (Bluetooth/Vision):
- **Spremi Bocu**: `{"cmd": "ruka", "val": "boca"}`
- **Spremi Limenku**: `{"cmd": "ruka", "val": "limenka"}`
