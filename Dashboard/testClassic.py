import serial
import time

# --- UPUTE ZA POVEZIVANJE ---
# 1. Windows Settings -> Bluetooth & devices -> Add device -> Bluetooth -> HC-02
# 2. Unesi PIN: 1234
# 3. Otvori "Bluetooth Settings" -> "More Bluetooth options" -> "COM Ports" tab
# 4. Pronađi COM port s smjerom "Outgoing" za 'HC-02 Dev B'. 
#    To je port koji treba upisati dolje (npr. 'COM5').

COM_PORT = 'COM5'  # <--- PROMIJENI OVO NAKON UPARIVANJA
BAUD_RATE = 9600

try:
    print(f"Otvaram {COM_PORT}...")
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=0.1)
    time.sleep(2) # Čekaj reset connectiona
    print(f"Uspješno otvoreno: {ser.name}")
    
    # Pošalji nešto za test
    ser.write(b"Test from Python\n")
    
    print("Slušam podatke... (Ctrl+C za prekid)")
    while True:
        if ser.in_waiting > 0:
            try:
                line = ser.readline().decode('utf-8').strip()
                if line:
                    print(f"Primljeno: {line}")
            except UnicodeDecodeError:
                # Ponekad prvi bajtovi budu smeće
                pass
                
        # Ovdje možeš slati podatke
        # ser.write(b"...")
        time.sleep(0.01)

except serial.SerialException as e:
    print(f"Greška pri otvaranju porta {COM_PORT}: {e}")
    print("Provjeri je li uređaj uparen i koristiš li točan COM port.")
except KeyboardInterrupt:
    print("Zatvaram...")
    if 'ser' in locals() and ser.is_open:
        ser.close()
