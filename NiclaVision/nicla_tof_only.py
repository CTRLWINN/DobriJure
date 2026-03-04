# nicla_tof_only.py
# Minimalni kod za iščitavanje isključivo TOF senzora na Nicla Vision ploči
# Šalje podatke preko UART-a na Arduino Mega

import time
import machine
import network
import sensor, image, usocket
from pyb import UART, LED

try:
    from vl53l1x import VL53L1X
except ImportError:
    print("Upozorenje: vl53l1x modul nije pronađen u ovom firmware-u!")

# --- Konfiguracija UART-a ---
UART_BAUDRATE = 9600
uart = UART(9, UART_BAUDRATE) # UART 9 koristi PA9 (TX) i PA10 (RX) na Nicla Vision OpenMV fw-u!
uart.init(UART_BAUDRATE, bits=8, parity=None, stop=1)

# --- LED Indikatori ---
led_red = LED(1)
led_green = LED(2)
led_blue = LED(3)

# --- WiFi Konfiguracija ---
SSID = "Robotika" # upisati WiFi SSID
KEY  = "1L0v3R0b0ts!" # upisati password
HOST = ''
PORT = 8080

print("Spajanjem na WiFi...")
wlan = network.WLAN(network.STA_IF)
wlan.active(True)

while not wlan.isconnected():
    try:
        print("Povezivanje na {}...".format(SSID))
        wlan.connect(SSID, KEY)
        
        # Wait for connection (timeout 10s)
        start = time.time()
        while not wlan.isconnected():
            led_blue.on()
            time.sleep_ms(250)
            led_blue.off()
            time.sleep_ms(250)
            if time.time() - start > 10:
                print("Connection timeout.")
                break
        
        if wlan.isconnected():
            break
            
        print("Ponovni pokusaj spajanja za 2 sekunde...")
        time.sleep(2)
    except Exception as e:
        print("WiFi Error:", e)
        time.sleep(2)

nicla_ip = "0.0.0.0"
if wlan.isconnected():
    nicla_ip = wlan.ifconfig()[0]
    print("WiFi spojen! IP:", nicla_ip)
    # Dugi plavi bljesak za oznaku spajanja
    led_blue.on() 
    time.sleep_ms(2000)
    led_blue.off()

# --- Inicijalizacija TOF senzora ---
tof = None
print("Pokusavam inicijalizirati TOF senzor na I2C(2)...")
try:
    i2c = machine.I2C(2)
    tof = VL53L1X(i2c)
    print("TOF senzor USPJEŠNO inicijaliziran!")
    led_green.on()
    time.sleep_ms(500)
    led_green.off()
except Exception as e:
    print("GREŠKA pri inicijalizaciji TOF senzora:", e)
    led_red.on()

# --- Inicijalizacija Kamere ---
print("Ucitavanje kamere...")
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA) # 320x240
sensor.skip_frames(time = 2000)

# --- Server Socket (MJPEG Stream) ---
s = usocket.socket(usocket.AF_INET, usocket.SOCK_STREAM)
s.bind((HOST, PORT))
s.listen(1)
s.settimeout(0.0) # Non-blocking accept
print("MJPEG Streamer spreman na portu", PORT)

client_socket = None

def send_mjpeg_header(client):
    client.send("HTTP/1.1 200 OK\r\n" \
                "Server: OpenMV\r\n" \
                "Content-Type: multipart/x-mixed-replace;boundary=openmv\r\n" \
                "Cache-Control: no-cache\r\n" \
                "Pragma: no-cache\r\n\r\n")

def send_mjpeg_frame(client, img):
    try:
        cframe = img.to_jpeg(quality=30)
        header = "\r\n--openmv\r\n" \
                 "Content-Type: image/jpeg\r\n"\
                 "Content-Length:"+str(len(cframe))+"\r\n\r\n"
        client.send(header)
        client.send(cframe)
    except Exception as e:
        print("MJPEG Send Error:", e)
        return False
    return True

# --- Glavna Petlja ---
while True:
    img = sensor.snapshot()
    
    if tof:
        try:
            # Ukljuci zelenu LEDicu kao znak očitavanja
            led_green.on()
            
            udaljenost_mm = tof.read()
            
            # Formatiramo poruku za Arduino ("TOF:150", "IP:192.168...")
            poruka = "TOF:{}\r\nIP:{}\r\n".format(udaljenost_mm, nicla_ip)
            
            # Šaljemo preko UART kabela na Megu kao prave bajtove
            uart.write(poruka.encode('utf-8'))
            
            # Iskljuci zelenu LEDicu
            led_green.off()
            
            print("UART poslao:", poruka.replace("\n", " "))
        except Exception as e:
            print("Greška kod očitavanja:", e)

    # --- MJPEG Stream ---
    if s:
        if not client_socket:
            try:
                client_socket, addr = s.accept()
                print("Client spojen:", addr)
                client_socket.settimeout(3.0)
                send_mjpeg_header(client_socket)
            except OSError:
                pass # Nema novih konekcija
        
        if client_socket:
            if not send_mjpeg_frame(client_socket, img):
                print("Client odspojen.")
                client_socket.close()
                client_socket = None

    # Pauza od 1000 milisekundi -> 1 FPS snimka i 1 put u sekundi UART
    time.sleep_ms(1000)
