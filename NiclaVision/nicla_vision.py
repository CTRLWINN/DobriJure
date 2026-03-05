# nicla_vision.py
# Platform: Arduino Nicla Vision (MicroPython / OpenMV)

import sensor, image, time, pyb, network, usocket, machine
from pyb import UART, LED

# --- Wi-Fi Konfiguracija ---
SSID = "Robotika" 
KEY  = "1L0v3R0b0ts!"
HOST = ''
PORT = 8080

# --- Konfiguracija UART-a ---
UART_BAUDRATE = 9600
uart = UART(9, UART_BAUDRATE, bits=8, parity=None, stop=1)

# --- LED ---
led_red = LED(1)
led_green = LED(2)
led_blue = LED(3)

# --- Inicijalizacija Kamere ---
sensor.reset()
sensor.set_pixformat(sensor.RGB565)

# Prisiljavanje na korištenje punog dijela senzora (Wide FOV) prije downscale-a
try:
    sensor.ioctl(sensor.IOCTL_SET_FOV_WIDE, True)
except Exception as e:
    print("Wide FOV flag not supported directly, using default FOV.")

sensor.set_framesize(sensor.QVGA) # 320x240 (VGA (640x480) uzrokuje Frame buffer overflow jer izlazimo iz RAM-a)

# Rotacija 180 stupnjeva jer je kamera naopako
sensor.set_hmirror(True)
sensor.set_vflip(True)

sensor.skip_frames(time=2000)
sensor.set_auto_gain(False)
sensor.set_auto_whitebal(False)

# --- Inicijalizacija TOF senzora na I2C(2) ---
# NAPOMENA: Sada se inicijalizira *NAKON* kamere, jer je prije 
# (kad se inicijalizirao prvi) izazivao "sensor detached" grešku!
try:
    from vl53l1x import VL53L1X
    i2c = machine.I2C(2)
    tof = VL53L1X(i2c)
    print("TOF Uspjesno inicijaliziran!")
except Exception as e:
    print("TOF Init Error:", e)
    tof = None

# --- Wi-Fi Spajanje ---
print("Spajanje na WiFi...")
wlan = network.WLAN(network.STA_IF)
wlan.active(True)

if not wlan.isconnected():
    wlan.connect(SSID, KEY)
    # Nećemo blokirati ovdje zauvijek! 
    # Sustav će pokušavati u pozadini preko wlan.connect() 
    start = time.time()
    while not wlan.isconnected() and time.time() - start < 5:
        time.sleep_ms(500)
        print("Spajam...")

if wlan.isconnected():
    ip_address = wlan.ifconfig()[0]
    print("WiFi Spojen! IP Adresa:", ip_address)
    led_blue.on()
    time.sleep_ms(1000)
    led_blue.off()
else:
    print("WiFi nije spojen. Provjeri mrezu.")

# --- Server Socket za MJPEG Stream ---
s = usocket.socket(usocket.AF_INET, usocket.SOCK_STREAM)
s.bind((HOST, PORT))
s.listen(1)
s.settimeout(0.0) # Non-blocking accept
print("MJPEG Streamer radi na portu", PORT)

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
        return True
    except Exception as e:
        return False


# --- Konfiguracija za Praćenje Ceste ---
# Trazimo rubove uz pomoc linearne regresije nad thresholdom.
# Puno otpornije na rasvjetu nego RGB. Prvi je L_min, L_max. 
THRESHOLD_CRNA_GRAY = [(0, 45)] # 0 je skroz crno, 45 je mracno sivo

# Lijeva polovica ekrana, rezemo zadnjih 40px (motor)
ROI_LIJEVI = (0, 60, 160, 140)   # x, y, w, h
# Desna polovica ekrana, rezemo zadnjih 40px (motor)
ROI_DESNI = (160, 60, 160, 140)

# Pomoc za crtanje strelice
def nacrtaj_strelicu(img, x1, y1, x2, y2, color):
    img.draw_line((int(x1), int(y1), int(x2), int(y2)), color=color, thickness=2)
    # Nacrtaj glavu (jako pojednostavljeno)
    img.draw_circle(int(x2), int(y2), 4, color=color, fill=True)

# --- Varijable stanja ---
# 0 -> cekanje naredbe
# 1 -> mjerenje udaljenosti ('u')
# 2 -> praćenje ceste ('c')
# 3 -> citanje QR koda ('q')
trenutni_mod = 0 

clock = time.time()
zadnje_slanje_ip = time.time()

while True:
    img = sensor.snapshot()
    
    # 1. Čitanje naredbi sa UART-a
    if uart.any():
        komanda = uart.read(1).decode('utf-8')
        if komanda == '0':
            trenutni_mod = 0
            # Vrati normalne boje i rezoluciju
            sensor.set_framesize(sensor.QVGA)
            sensor.set_windowing((320, 240))
            sensor.set_pixformat(sensor.RGB565)
            print("Stanje: 0 (Cekanje)")
        elif komanda == 'u':
            trenutni_mod = 1
            sensor.set_framesize(sensor.QVGA)
            sensor.set_windowing((320, 240))
            sensor.set_pixformat(sensor.RGB565)
            print("Stanje: 1 (Mjerenje udaljenosti)")
        elif komanda == 'c':
            trenutni_mod = 2
            # Za regresiju slika mora biti grayscale
            sensor.set_framesize(sensor.QVGA)
            sensor.set_windowing((320, 240))
            sensor.set_pixformat(sensor.GRAYSCALE)
            print("Stanje: 2 (Pracenje Ceste - GRAYSCALE Regression)")
        elif komanda == 'q':
            trenutni_mod = 3
            # Kamera ostaje u QVGA/RGB565 - ne mijenjamo rezoluciju!
            print("Stanje: 3 (QR Kod)")

    # 1.5 Slanje IP/Konekcijskog statusa Arduinu svake sekunde (ako nismo u brzom modu poput voznje)
    trenutno_vrijeme = time.time()
    if trenutno_vrijeme - zadnje_slanje_ip > 1.0:
        zadnje_slanje_ip = trenutno_vrijeme
        if wlan.isconnected():
             uart.write("IP:{}\r\n".format(wlan.ifconfig()[0]).encode())
        else:
             uart.write("IP:\r\n".encode()) # Šaljemo prazno (Arduino onda parsira kao "")

    # 2. Izvršavanje akcija ovisno o stanju
    
    if trenutni_mod == 0:
        img.draw_string(10, 10, "MOD: CEKANJE (0)", color=(255,165,0), scale=2)
        
    elif trenutni_mod == 1:
        img.draw_string(10, 10, "MOD: TOF (1)", color=(0,255,255), scale=2)
        if tof:
            try:
                # vl53l1x.read() standardno na OpenMV vraca vrijednost u milimetrima!
                udaljenost_mm = tof.read()
                uart.write("TOF:{}\r\n".format(udaljenost_mm).encode())
                img.draw_string(10, 40, "Ud: {} mm".format(udaljenost_mm), color=(255,255,255), scale=2)
            except: pass
                
    elif trenutni_mod == 2:
        img.draw_string(10, 10, "MOD: LANE ASSIST (EDGE)", color=255, scale=2)
        
        # Prikaz regija skeniranja (kako bi vidio rez motora)
        img.draw_rectangle(ROI_LIJEVI, color=100)
        img.draw_rectangle(ROI_DESNI, color=100)
        
        # Pronađi linearnu pravocrtnu liniju za lijevi i desni rub staze
        reg_lijeva = img.get_regression(THRESHOLD_CRNA_GRAY, roi=ROI_LIJEVI, robust=True)
        reg_desna = img.get_regression(THRESHOLD_CRNA_GRAY, roi=ROI_DESNI, robust=True)
        
        centar_x = 160.0 # centar ekrana (širina 320 / 2)
        greska = 0.0
        
        if reg_lijeva:
            img.draw_line(reg_lijeva.line(), color=0, thickness=2) # Nacrtaj crnu liniju na rub
        if reg_desna:
            img.draw_line(reg_desna.line(), color=0, thickness=2)
            
        # Logika izracuna sredine ceste
        # Trazimo X koordinatu obje linije na sredini visine ekrana (npr. y=130)
        y_cilj = 130
        
        cx_lijeva = 0
        cx_desna = 320
        
        if reg_lijeva and reg_desna:
            # Izracunaj x iz formule pravca y = a*x + b 
            # OpenMV nam daje kut i rho, najbolje je uzeti točku na dnu bounding boxa kao referencu
            x_L = reg_lijeva.x2() if y_cilj > reg_lijeva.y1() else reg_lijeva.x1()
            x_R = reg_desna.x2() if y_cilj > reg_desna.y1() else reg_desna.x1()
            sredina_ceste = (x_L + x_R) / 2.0
            greska = (sredina_ceste - centar_x) / centar_x
        elif reg_lijeva and not reg_desna:
            # Vidi samo lijevu liniju. Pretpostavka je da smo previše lijevo. Guraj malo desno.
            greska = 0.3
            sredina_ceste = 250
        elif not reg_lijeva and reg_desna:
            # Vidi samo desnu liniju. Guraj u lijevo
            greska = -0.3
            sredina_ceste = 70
        else:
            greska = 0.0
            sredina_ceste = 160
            
        # Ograničenje greške na -1 do 1
        greska = max(-1.0, min(1.0, greska))
        
        # Crtanje Vektora Strelica
        # Pocnite iz centra na dnu regije skeniranja (160, 200)
        start_x, start_y = (160, 200)
        
        # Cilj zrcalno okrenut ako je greska -0.5, tocka x je 80 (lijevo)
        cilj_x = 160 + (greska * 160)
        cilj_y = 100 # vrh ekrana negdje
        
        # Draw crnu / bijelu ovisno o okruzenju
        nacrtaj_strelicu(img, start_x, start_y, cilj_x, cilj_y, color=255)
        
        img.draw_string(10, 30, "Error: {:.2f}".format(greska), color=255, scale=2)
        
        uart.write("LINE:{:.2f}\r\n".format(greska).encode())
        
    elif trenutni_mod == 3:
        img.draw_string(10, 10, "MOD: QR SKANER (3)", color=(255,0,0), scale=2)
        qrs = img.find_qrcodes()
        if qrs:
            for qr in qrs:
                img.draw_rectangle(qr.rect(), color=(0, 255, 0))
                
                # Pretvori višelinijski payload u jednu liniju odvojenu s točka-zarezom
                redovi = qr.payload().replace("\r", "").split("\n")
                formatirani_payload = ";".join([r.strip() for r in redovi if r.strip()])
                
                img.draw_string(qr.rect()[0], qr.rect()[1]-20, "QR Ucitano", color=(0,255,0), scale=2)
                
                uart.write("QR:{}\r\n".format(formatirani_payload).encode())
                print("QR Procitan formatirano:", formatirani_payload)
                
                # Pali zelenu LED diodu 1 sekundu i gasi ju
                led_green.on()
                time.sleep_ms(1000)
                led_green.off()
                
                # Vrati se u mod cekanja (kamera ostaje u QVGA - nema resetiranja)
                trenutni_mod = 0
                print("Stanje: 0 (Cekanje)")
                
                # Izlazimo iz petlje nakon prvog uspješnog QR-a
                break 
        else:
            led_green.off()

    # 3. MJPEG Stream Slanje
    if s:
        if not client_socket:
            try:
                client_socket, addr = s.accept()
                print("Klijent spojen na stream:", addr)
                client_socket.settimeout(3.0)
                send_mjpeg_header(client_socket)
            except OSError:
                pass # Nema novog klijenta

        if client_socket:
            uspjeh = send_mjpeg_frame(client_socket, img)
            if not uspjeh:
                print("Klijent iskljucen.")
                client_socket.close()
                client_socket = None
