# JSON Komunikacijski Protokol

Ovaj dokument opisuje format JSON poruka koje se razmjenjuju između PC Dashboarda i Robota.

## Smjer: PC -> Robot (TX)

Sve komande šalju se kao JSON objekt u jednoj liniji, završeno znakom `\n`.

### Osnovne Komande
| Komanda | JSON Primjer | Opis |
| :--- | :--- | :--- |
| **Start** | `{"cmd": "start"}` | Pokreće robota (Enable motors). |
| **Stop** | `{"cmd": "stop"}` | Zaustavlja robota (Disable motors). |
| **Reset Pozicije** | `{"cmd": "reset_pose"}` | Postavlja X,Y,Theta na 0. |

### Ručno Upravljanje (WASD)
Slanje direktnih PWM/Brzina vrijednosti za lijevi i desni motor.
- **Format:** `{"cmd": "manual", "l": <brzina>, "r": <brzina>}`
- **Primjer:** `{"cmd": "manual", "l": 100, "r": 100}` (Naprijed)

### Konfiguracija
| Komanda | JSON Primjer | Opis |
| :--- | :--- | :--- |
| **PID Postavke** | `{"cmd": "set_pid", "p": 1.5, "i": 0.01, "d": 0.5}` | Postavlja PID konstante. |
| **Kalibracija IMU** | `{"cmd": "cal_imu"}` | Pokreće kalibraciju žiroskopa. |
| **Spremi EEPROM** | `{"cmd": "save_eeprom"}` | Sprema trenutne postavke u memoriju. |

### Misija (Sekvencer)
| Komanda | JSON Primjer | Opis |
| :--- | :--- | :--- |
| **Vozi Ravno** | `{"cmd": "straight", "val": 100}` | Vozi 100 cm ravno. |
| **Rotiraj** | `{"cmd": "turn", "val": 90}` | Rotiraj za 90 stupnjeva udesno. |
| **Ruka** | `{"cmd": "arm", "val": "boca"}` | Pokreni sekvencu ruke za bocu. |

### Upiti
- **Dohvati Poziciju:** `{"cmd": "get_pose"}` -> Robot odgovara sa Snapshot porukom.

---

## Smjer: Robot -> PC (RX)

Robot šalje telemetriju periodički ili na zahtjev.

### Telemetrija (Periodička - npr. 10Hz)
Sadrži osnovne podatke za dashboard.
```json
{
  "enc_l": 1250,      // Brojač lijevog enkodera
  "enc_r": 1245,      // Brojač desnog enkodera
  "spd": 0.45,        // Trenutna brzina (m/s)
  "head": 12.5,       // Trenutni smjer (stupnjevi)
  "bat": 11.2,        // Napon baterije (V)
  "status": "Running" // Status text
}
```

### Snapshot (Odgovor na "get_pose")
Koristi se u "Učenje" modu za snimanje točaka.
```json
{
  "pose_x": 1.25,     // X koordinata (m)
  "pose_y": 0.50,     // Y koordinata (m)
  "pose_th": 90.0,    // Orijentacija (stupnjevi)
  "type": "pose_snapshot"
}
```
