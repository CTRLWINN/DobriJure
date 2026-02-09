import asyncio
from bleak import BleakScanner, BleakClient

# --- DETEKTIRANI UUID-ovi (ISSC Chipset) ---
# Service: 49535343-fe7d-4ae5-8fa9-9fafd205e455
UART_RX_CHAR_UUID = "49535343-1e4d-4bd9-ba61-23c647249616" # Notify
UART_TX_CHAR_UUID = "49535343-8841-43f4-a8d4-ecbe34729bb3" # Write (pretpostavka)

async def run():
    print("Tražim HC-02...")
    devices = await BleakScanner.discover()
    
    hc02_device = None
    for d in devices:
        if d.name and "HC-02" in d.name:
            hc02_device = d
    
    if not hc02_device:
        print("HC-02 nije pronađen!")
        return

    print(f"Spajam se na {hc02_device.address}...")
    
    async with BleakClient(hc02_device.address) as client:
        print(f"Spojen: {client.is_connected}")
        
        def notification_handler(sender, data):
            print(f"[ARDUINO]: {data.decode('utf-8', errors='ignore')}")

        try:
            print(f"Pretplaćujem se na notify: {UART_RX_CHAR_UUID}")
            await client.start_notify(UART_RX_CHAR_UUID, notification_handler)
            print("Slušam podatke... (Pošalji nešto s Arduino Serial Monitora!)")
            
            # Pošalji "ping"
            print(f"Šaljem 'ping' na: {UART_TX_CHAR_UUID}")
            try:
                await client.write_gatt_char(UART_TX_CHAR_UUID, b"ping\n")
            except Exception as e:
                print(f"Ne mogu pisati (ovo je OK ako samo testiramo RX): {e}")

            while True:
                await asyncio.sleep(1)
                
        except Exception as e:
            print(f"GREŠKA: {e}")

asyncio.run(run())