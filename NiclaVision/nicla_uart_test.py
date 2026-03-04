import time
from machine import UART
from pyb import LED

# UART 9 (PA9 TX, PA10 RX) na 9600 s eksplicitnim postavkama
uart = UART(9, 9600, bits=8, parity=None, stop=1)

led_blue = LED(3)

print("Nicla -> Arduino Test Slanja Startan (9600 bps)")

brojac = 1
while True:
    poruka = "TOF:{}\r\n".format(brojac * 10)

    # Šaljemo poruku
    uart.write(poruka.encode('utf-8'))
    print("Poslano na TX (PA9):", poruka.strip())

    led_blue.on()
    time.sleep_ms(50)
    led_blue.off()

    # Čitamo poruku ako ima išta na UART-u (RX pin)
    if uart.any():
        primljeno = uart.read()
        print("Primljeno sa RX (PA10):", primljeno)

    brojac += 1
    time.sleep_ms(1000)
