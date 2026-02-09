# nicla_vision.py
# WorldSkills Croatia 2026 - Perception Layer
# Platform: Arduino Nicla Vision (MicroPython / OpenMV)

import sensor, image, time, pyb, network, usocket, sys
from pyb import UART, LED

# --- Wi-Fi Konfiguracija ---
SSID = "********" // upisati WiFi SSID
KEY  = "********" // upisati password
HOST = '' 
PORT = 8080

# --- Konfiguracija ---
UART_BAUDRATE = 115200

# --- Konfiguracija Boja (LAB Thresholds) ---
# --- Konfiguracija Boja (LAB Thresholds) ---
# NAPOMENA: Ove vrijednosti MORATE kalibrirati u OpenMV IDE (Tools -> Machine Vision -> Threshold Editor)
# Format: (L_min, L_max, A_min, A_max, B_min, B_max)

# 1. PET Boca (Prozirna) -> NE MOŽEMO detektirati prozirnu plastiku.
# RJEŠENJE: Ciljamo ČEP (npr. Plavi/Crveni) ili ETIKETU (npr. Zelena Sprite).
# Ovdje namjestite boju čepa ili etikete!
THRESHOLD_BOCA_CEP   = (30, 100, -64, -8, -32, 32)   # Generic Green (Zamijeniti s bojom čepa!)

# 2. Limenka (Obično CocaCola - Crvena ili Pepsi - Plava) -> Ciljamo CRVENU
THRESHOLD_LIMENKA = (30, 100, 15, 127, 15, 127)  # Generic Red

# 3. Spužvica (Obično Žuta) -> Ciljamo ŽUTU
THRESHOLD_SPUZVA = (60, 100, -10, 10, 40, 80)    # Generic Yellow

THRESHOLDS = [
    (THRESHOLD_BOCA_CEP, "BOCA",    (0, 255, 0)),   # Green Box
    (THRESHOLD_LIMENKA,"LIMENKA", (255, 0, 0)),   # Red Box
    (THRESHOLD_SPUZVA, "SPUZVA",  (255, 255, 0))  # Yellow Box
]

# --- Inicijalizacija ---
led_red = LED(1)
led_green = LED(2)
led_blue = LED(3)

# Treptaj za start
led_red.on()
time.sleep_ms(200) # Cekaj 200 milisekundi, ne sekundi!
led_red.off()

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA) # 320x240
sensor.skip_frames(time = 2000)   # Čekaj da se senzor stabilizira
sensor.set_auto_gain(False)       # Isključi za stabilnu detekciju boje
sensor.set_auto_whitebal(False)   # Isključi za stabilnu detekciju boje

# UART 3 na Nicla Vision se koristi za komunikaciju prema van (TX=P3, RX=P4 na headeru?)
# Provjeriti mapiranje. Nicla Vision obično koristi UART(4) ili UART(1) ovisno o FW.
# Prema pinoutu, koristimo UART sa 115200.
uart = UART(4, UART_BAUDRATE) # UART 4 je često na standardnim pinovima
uart.init(115200, bits=8, parity=None, stop=1)

clock = time.clock()

# --- Wi-Fi Main ---
print("Initializing WiFi...")
wlan = network.WLAN(network.STA_IF)
wlan.active(True)

while not wlan.isconnected():
    try:
        print(f"Connecting to {SSID}...")
        wlan.connect(SSID, KEY)
        
        # Wait for connection (timeout 10s locally)
        start = time.time()
        while not wlan.isconnected():
            time.sleep_ms(500)
            if time.time() - start > 10:
                print("Connection timeout.")
                break
        
        if wlan.isconnected():
            break
            
        print("Retrying in 2 seconds...")
        time.sleep(2)
    except Exception as e:
        print(f"WiFi Error: {e}")
        time.sleep(2)

print("WiFi Connected. IP:", wlan.ifconfig()[0])
led_blue.on()
time.sleep_ms(500)
led_blue.off()

# Server Socket
s = usocket.socket(usocket.AF_INET, usocket.SOCK_STREAM)
s.bind((HOST, PORT))
s.listen(1)
s.settimeout(0.0) # Non-blocking accept
print("MJPEG Streamer running on port", PORT)

client_socket = None

def send_mjpeg_header(client):
    client.send("HTTP/1.1 200 OK\r\n" \
                "Server: OpenMV\r\n" \
                "Content-Type: multipart/x-mixed-replace;boundary=openmv\r\n" \
                "Cache-Control: no-cache\r\n" \
                "Pragma: no-cache\r\n\r\n")

def send_mjpeg_frame(client, img):
    try:
        # Use to_jpeg instead of compressed()
        cframe = img.to_jpeg(quality=30)
        header = "\r\n--openmv\r\n" \
                 "Content-Type: image/jpeg\r\n"\
                 "Content-Length:"+str(len(cframe))+"\r\n\r\n"
        client.send(header)
        client.send(cframe)
    except Exception as e:
        print("MJPEG Send Error:", e)
        return False # Socket error
    return True

# --- Funkcije ---

def send_uart(tag, data):
    msg = "{}:{}\n".format(tag, data)
    uart.write(msg)
    print("Sent: " + msg.strip())

while(True):
    clock.tick()
    img = sensor.snapshot()
    
    # --- 0. Provjera UART poruka (Konfiguracija) ---
    if uart.any():
        line = uart.readline()
        if line:
            try:
                msg = line.decode().strip()
                # Format: SET:TIP,l_min,l_max,a_min,a_max,b_min,b_max
                # Primjer: SET:BOCA,30,100,-64,-8,-32,32
                if msg.startswith("SET:"):
                    parts = msg.split(":")[1].split(",")
                    if len(parts) == 7:
                        tip = parts[0] # BOCA, LIMENKA, SPUZVA
                        vals = (int(parts[1]), int(parts[2]), int(parts[3]), int(parts[4]), int(parts[5]), int(parts[6]))
                        
                        print("Updating " + tip + " to: " + str(vals))
                        
                        # Ažuriraj listu
                        new_thresholds = []
                        updated = False
                        for thresh, label, color in THRESHOLDS:
                            if label == tip:
                                new_thresholds.append((vals, label, color))
                                updated = True
                            else:
                                new_thresholds.append((thresh, label, color))
                        
                        if updated:
                            THRESHOLDS = new_thresholds
                            # Trepni plavo za potvrdu
                            led_blue.on()
                            time.sleep_ms(100)
                            led_blue.off()
            except Exception as e:
                print("UART Error:", e)

    # 1. QR Kod Detekcija
    qrs = img.find_qrcodes()
    if qrs:
        led_green.on()
        for qr in qrs:
            # Nacrtaj pravokutnik oko QR koda
            img.draw_rectangle(qr.rect(), color = (0, 255, 0))
            payload = qr.payload()
            send_uart("QR", payload)
            # Debounce da ne salje milijun puta isti
            time.sleep_ms(500)
    else:
        led_green.off()
        
    # 2. Detekcija Objekata (Colors)
    # Trazimo sve boje definriane u THRESHOLDS
    
    current_best_blob = None
    detected_type = None
    
    for thresh, label, color in THRESHOLDS:
        blobs = img.find_blobs([thresh], pixels_threshold=200, area_threshold=200, merge=True)
        if blobs:
            # Nadji najveci blob ove boje
            largest = max(blobs, key=lambda b: b.pixels())
            
            # Provjeri je li ovo najveci objekt generalno u ovom frameu? 
            # (Jednostavna logika: prikazujemo sve, ali saljemo najveci)
            img.draw_rectangle(largest.rect(), color=color)
            img.draw_string(largest.x(), largest.y() - 10, label, color=color)
            
            if current_best_blob is None or largest.pixels() > current_best_blob.pixels():
                current_best_blob = largest
                detected_type = label

    # Salji podatke o najvecem detektiranom objektu
    if current_best_blob:
        img.draw_cross(current_best_blob.cx(), current_best_blob.cy())
        
        # Format: OBJ:TIP,x,y,povrsina
        # Centar ekrana je cca 160, 120 (za QVGA 320x240)
        # Saljemo odstupanje od centra za PID? Ili apsolutne koordinate?
        # Saljemo tip i X koordinatu za rotaciju robota.
        
        data = "{},{},{}".format(detected_type, current_best_blob.cx(), current_best_blob.pixels())
        send_uart("OBJ", data)
        
    # Debounce / Throttle
    time.sleep_ms(50) 

    # --- 3. MJPEG Stream ---
    if s:
        # 1. Prihvati novog klijenta ako ga nema
        if not client_socket:
            try:
                client_socket, addr = s.accept()
                print("Client connected:", addr)
                client_socket.settimeout(3.0) 
                send_mjpeg_header(client_socket)
            except OSError:
                pass # Nema novih
        
        # 2. Salji frame ako imamo klijenta
        if client_socket:
            if not send_mjpeg_frame(client_socket, img):
                print("Client disconnected.")
                client_socket.close()
                client_socket = None

    # print(clock.fps())
