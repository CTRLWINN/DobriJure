import customtkinter as ctk
import tkinter as tk
from tkinter import messagebox
import threading
import asyncio
from bleak import BleakScanner, BleakClient
import time
import requests
import json
from PIL import Image, ImageTk
from io import BytesIO
import os
import tkinter.filedialog as filedialog

import socket

# Configuration
DEFAULT_IP = "192.168.0.7"

# UUID-ovi za HC-02 (ISSC)
UART_RX_CHAR_UUID = "49535343-1e4d-4bd9-ba61-23c647249616" # Notify
UART_TX_CHAR_UUID = "49535343-8841-43f4-a8d4-ecbe34729bb3" # Write

ctk.set_appearance_mode("Dark")
ctk.set_default_color_theme("blue")

class RobotController:
    def __init__(self, loop):
        self.loop = loop
        self.client = None
        self.connected = False
        self.connected = False
        self.on_telemetry_callback = None
        self.on_disconnect_callback = None
        self.on_disconnect_callback = None
        self.rx_buffer = ""
        
        # Sync control
        self.response_event = threading.Event()
        self.last_response = None

    async def connect_ble(self):
        print("Skeniram BLE uređaje...")
        devices = await BleakScanner.discover()
        target = None
        for d in devices:
            if d.name and "HC-02" in d.name:
                target = d
                break
        
        if not target:
            print("HC-02 nije pronađen.")
            return False

        print(f"Povezivanje na {target.name}...")
        self.client = BleakClient(target.address, disconnected_callback=self.handle_disconnect)
        
        try:
            await self.client.connect()
            self.connected = True
            print("BLE Povezano!")
            await self.client.start_notify(UART_RX_CHAR_UUID, self.notification_handler)
            return True
        except Exception as e:
            print(f"Greška pri povezivanju: {e}")
            self.connected = False
            return False

    async def disconnect_ble(self):
        if self.client:
            await self.client.disconnect()
            self.connected = False
            print("BLE Odspojeno.")

    def handle_disconnect(self, client):
        print("BLE Disconnected event!")
        self.connected = False
        if self.on_disconnect_callback:
            self.on_disconnect_callback()

    def notification_handler(self, sender, data):
        try:
            chunk = data.decode('utf-8', errors='ignore')
            print(f"DEBUG RX CHUNK: {repr(chunk)}") # Configurable debug
            self.rx_buffer += chunk
            
            if "\n" in self.rx_buffer:
                lines = self.rx_buffer.split("\n")
                
                for line in lines[:-1]:
                    line = line.strip()
                    if not line: continue
                    
                    # Relaxed matching: verify contains STATUS:
                    if "STATUS:" in line:
                        # Extract from STATUS: onwards
                        try:
                            clean_line = line[line.find("STATUS:"):]
                            parts = clean_line.split(":")[1].split(",")
                            if len(parts) >= 9:
                                data_dict = {
                                    "cm": parts[0], "pL": parts[1], "pR": parts[2],
                                    "arm": parts[3],
                                    "usF": parts[4], "usB": parts[5], "usL": parts[6], "usR": parts[7],
                                    "ind": parts[8],
                                    "yaw": parts[9] if len(parts) > 9 else "0",
                                    "gyro": parts[10] if len(parts) > 10 else "0",
                                    "gz": parts[11] if len(parts) > 11 else "0",
                                    "tof": parts[12] if len(parts) > 12 else "0",
                                    "ip": parts[13] if len(parts) > 13 else "0.0.0.0",
                                    "destM": parts[14] if len(parts) > 14 else "-",
                                    "destP": parts[15] if len(parts) > 15 else "-",
                                    "destS": parts[16] if len(parts) > 16 else "-"
                                }
                                if self.on_telemetry_callback:
                                    self.on_telemetry_callback(data_dict)
                        except: pass
                    
                    elif "{" in line and "}" in line:
                         # Likely JSON response
                         try:
                             start = line.find("{")
                             end = line.rfind("}") + 1
                             json_str = line[start:end]
                             data = json.loads(json_str)
                             if "status" in data:
                                 self.last_response = data
                                 self.response_event.set()
                         except: pass
                         
                self.rx_buffer = lines[-1]
                
        except Exception as e:
            print(f"RX Error: {e}")

    def send_command(self, cmd):
        if self.connected and self.client:
             asyncio.run_coroutine_threadsafe(self.write_ble(cmd), self.loop)

    def send_command_and_wait(self, cmd, timeout=15.0):
        self.response_event.clear()
        self.last_response = None
        
        self.send_command(cmd)
        
        # Wait for response
        if self.response_event.wait(timeout):
            return self.last_response
        return None

    async def write_ble(self, cmd):
        if self.client and self.connected:
            try:
                data = (cmd + "\n").encode('utf-8')
                await self.client.write_gatt_char(UART_TX_CHAR_UUID, data)
                print(f"TX: {cmd}")
            except Exception as e:
                print(f"TX Fail: {e}")

class WiFiStatusChecker:
    def __init__(self, app, ip, interval=2.0):
        self.app = app
        self.ip = ip
        self.interval = interval
        self.running = False
        self.thread = None

    def start(self):
        if not self.running:
            self.running = True
            self.thread = threading.Thread(target=self.loop, daemon=True)
            self.thread.start()

    def loop(self):
        while self.running:
            status = False
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(1.0)
                result = sock.connect_ex((self.ip, 8080))
                if result == 0:
                    status = True
                sock.close()
            except:
                status = False
            
            self.app.update_wifi_status(status)
            time.sleep(self.interval)

class MJPEGViewer(ctk.CTkLabel):
    def __init__(self, master, url, **kwargs):
        super().__init__(master, text="Waiting for Stream...", **kwargs)
        self.url = url
        self.running = False
        self.thread = None
        self.frame_count = 0
        
    def start(self):
        if not self.running:
            self.running = True
            self.thread = threading.Thread(target=self.update_stream)
            self.thread.daemon = True
            self.thread.start()
            
    def stop(self):
        self.running = False

    def update_stream(self):
        print(f"Connecting to {self.url}...")
        while self.running:
            try:
                stream = requests.get(self.url, stream=True, timeout=5)
                if stream.status_code == 200:
                    print("Stream Connected!")
                    bytes_data = bytes()
                    for chunk in stream.iter_content(chunk_size=1024):
                        if not self.running: break
                        bytes_data += chunk
                        a = bytes_data.find(b'\xff\xd8') # JPEG Start
                        b = bytes_data.find(b'\xff\xd9') # JPEG End
                        if a != -1 and b != -1:
                            jpg = bytes_data[a:b+2]
                            bytes_data = bytes_data[b+2:] 
                            try:
                                image = Image.open(BytesIO(jpg))
                                # Resize to fit label
                                photo = ctk.CTkImage(image.resize((320, 240)), size=(320, 240))
                                self.configure(image=photo, text="")
                            except Exception as e:
                                pass
                else:
                    self.configure(text=f"Status: {stream.status_code}")
                    time.sleep(1)
            except Exception as e:
                self.configure(text=f"Reconnecting...")
                time.sleep(1) # Wait before retry

class DashboardApp(ctk.CTk):
    def __init__(self):
        super().__init__()
        self.title("WSC 2026 Mission Control (BLE Edition)")
        self.geometry("1000x700")
        
        # Async Loop setup
        self.loop = asyncio.new_event_loop()
        threading.Thread(target=self.start_async_loop, daemon=True).start()

        self.robot = RobotController(self.loop)
        self.robot.on_disconnect_callback = self.on_ble_disconnect
        
        # Layout
        self.grid_columnconfigure(1, weight=1)
        self.grid_rowconfigure(0, weight=1)
        
        # Sidebar
        self.sidebar = ctk.CTkFrame(self, width=200, corner_radius=0)
        self.sidebar.grid(row=0, column=0, sticky="nsew")
        
        self.logo_label = ctk.CTkLabel(self.sidebar, text="WSC ROBOT", font=ctk.CTkFont(size=20, weight="bold"))
        self.logo_label.grid(row=0, column=0, padx=20, pady=(20, 10))
        
        # Connection Box
        self.conn_frame = ctk.CTkFrame(self.sidebar)
        self.conn_frame.grid(row=1, column=0, padx=10, pady=10)
        
        self.btn_connect = ctk.CTkButton(self.conn_frame, text="Connect BLE (HC-02)", command=self.trigger_connect)
        self.btn_connect.pack(pady=5)
        
        # IP Address Input
        ctk.CTkLabel(self.sidebar, text="Nicla Vision IP:", font=ctk.CTkFont(size=12, weight="bold")).grid(row=2, column=0, padx=20, pady=(10, 0))
        self.entry_ip = ctk.CTkEntry(self.sidebar)
        self.entry_ip.grid(row=3, column=0, padx=20, pady=(0, 5))
        self.entry_ip.insert(0, DEFAULT_IP)
        
        self.btn_update_ip = ctk.CTkButton(self.sidebar, text="Potvrdi IP", command=self.update_nicla_ip)
        self.btn_update_ip.grid(row=4, column=0, padx=20, pady=(0, 20))
        
        # Tabs
        self.tabview = ctk.CTkTabview(self)
        self.tabview.grid(row=0, column=1, padx=20, pady=20, sticky="nsew")
        
        self.tab_calib = self.tabview.add("Kalibracija kamere")
        self.tab_manual = self.tabview.add("Učenje (Manual)")
        self.tab_auto = self.tabview.add("Autonomno")
        
        # State
        self.manual_speed = 100
        
        self.setup_calibration_tab()
        self.setup_manual_tab()
        self.setup_auto_tab()
        
        # WiFi Checker
        self.wifi_checker = WiFiStatusChecker(self, DEFAULT_IP)
        self.wifi_checker.start()

        # Hooks
        self.robot.on_telemetry_callback = self.update_telemetry
        self.ui_updater()

        # Key bindings for Manual Drive
        self.bind("<KeyPress>", self.on_key_press)

    def update_nicla_ip(self):
        new_ip = self.entry_ip.get().strip()
        self.wifi_checker.ip = new_ip
        messagebox.showinfo("IP Adresa", f"Nicla IP uspješno postavljen na:\n{new_ip}\nStatus povezivosti pratitelj ažurira svake dvije sekunde.")

    def start_async_loop(self):
        asyncio.set_event_loop(self.loop)
        self.loop.run_forever()

    def trigger_connect(self):
        if not self.robot.connected:
            self.btn_connect.configure(text="Connecting...", state="disabled")
            asyncio.run_coroutine_threadsafe(self.connect_ble_logic(), self.loop)
        else:
            asyncio.run_coroutine_threadsafe(self.disconnect_ble_logic(), self.loop)
            
    async def connect_ble_logic(self):
        success = await self.robot.connect_ble()
        if success:
            self.btn_connect.configure(text="Disconnect BLE", fg_color="red", state="normal")
        else:
            self.btn_connect.configure(text="Connect BLE (HC-02)", fg_color="blue", state="normal")
            
    async def disconnect_ble_logic(self):
        await self.robot.disconnect_ble()
        self.btn_connect.configure(text="Connect BLE (HC-02)", fg_color="blue", state="normal")

    def show_image_popup(self, jpg_data):
        try:
            top = ctk.CTkToplevel(self)
            top.title("Snapshot")
            top.geometry("400x300")
            
            image = Image.open(BytesIO(jpg_data))
            photo = ctk.CTkImage(image, size=(320, 240))
            
            lbl = ctk.CTkLabel(top, text="", image=photo)
            lbl.pack(expand=True, fill="both")
        except Exception as e:
            print(f"Popup error: {e}")

    def setup_calibration_tab(self):
        # Just Camera & Vision on this tab
        frame = self.tab_calib
        
        # Center container
        col2 = ctk.CTkFrame(frame)
        col2.pack(fill="both", expand=True, padx=20, pady=20)

        # --- VIZIJA ---
        ctk.CTkLabel(col2, text="Kamera & Boje", font=("Arial", 16, "bold")).pack(pady=5)
        
        # Nicla WiFi Status (Calibration Tab)
        self.wifi_status_frame = ctk.CTkFrame(col2, fg_color="transparent")
        self.wifi_status_frame.pack(pady=2)
        self.lbl_wifi_led = ctk.CTkLabel(self.wifi_status_frame, text="●", font=("Arial", 24), text_color="red")
        self.lbl_wifi_led.pack(side="left", padx=2)
        self.lbl_wifi_text = ctk.CTkLabel(self.wifi_status_frame, text=f"WiFi Disconnected ({DEFAULT_IP})")
        self.lbl_wifi_text.pack(side="left", padx=2)
        
        self.lbl_tof = ctk.CTkLabel(self.wifi_status_frame, text="TOF: - mm", text_color="cyan", font=("Arial", 14, "bold"))
        self.lbl_tof.pack(side="left", padx=15)

        self.lbl_nicla_ip = ctk.CTkLabel(self.wifi_status_frame, text="IP: -", text_color="yellow", font=("Arial", 14, "bold"))
        self.lbl_nicla_ip.pack(side="left", padx=15)

        # Video Stream 
        self.video_viewer = MJPEGViewer(col2, "", width=320, height=240, fg_color="black")
        self.video_viewer.pack(pady=5)
        # Button Row for Stream
        btn_row_calib = ctk.CTkFrame(col2, fg_color="transparent")
        btn_row_calib.pack(pady=5)
        ctk.CTkButton(btn_row_calib, text="Start Stream", command=self.start_video_stream).pack(side="left", padx=2)
        ctk.CTkButton(btn_row_calib, text="Prekini Stream", fg_color="red", command=self.stop_calibration_stream).pack(side="left", padx=2)
        
        # Manipulator (Servo) Controls
        ctk.CTkLabel(col2, text="Upravljanje Servo Motorima", font=("Arial", 16, "bold")).pack(pady=10)
        
        self.preset_names = [
            "PARKING", "SAFE", "VOZNJA", "PRIPREMA_QR", "CITANJE_QR", 
            "PRIPREMA_PICKUP", "PROVJERA_PICKUP", "PICKUP", "PROVJERA_METAL", 
            "VOZNJA_PICKUP", "OSTAVLJANJE_PRIPREMA", "OSTAVLJANJE_D1", 
            "OSTAVLJANJE_D2", "OSTAVLJANJE_D3", "PROVJERA_OSTAVLJANJE",
            "SAFE_PICKUP"
        ]
        
        # Kutovi definirani unutar aplikacije po novoj logici
        self.preset_angles = {
            "PARKING": [135, 164, 40, 100, 20],
            "SAFE": [135, 110, 80, 60, 90],
            "VOZNJA": [135, 140, 60, 150, 25],
            "PRIPREMA_QR": [190, 90, 60, 50, 40],
            "CITANJE_QR": [250, 80, 80, 55, 25],
            "PRIPREMA_PICKUP": [135, 80, 90, 40, 80],
            "PROVJERA_PICKUP": [135, 90, 90, 90, 90],
            "PICKUP": [135, 65, 130, 70, 145],
            "PROVJERA_METAL": [185, 105, 50, 70, 145],
            "VOZNJA_PICKUP": [130, 150, 55, 20, 145],
            "OSTAVLJANJE_PRIPREMA": [135, 90, 90, 90, 90],
            "OSTAVLJANJE_D1": [135, 90, 90, 90, 90],
            "OSTAVLJANJE_D2": [135, 90, 90, 90, 90],
            "OSTAVLJANJE_D3": [135, 90, 90, 90, 90],
            "PROVJERA_OSTAVLJANJE": [135, 90, 90, 90, 90],
            "SAFE_PICKUP": [135, 110, 80, 60, 145]
        }
        
        row_preset = ctk.CTkFrame(col2)
        row_preset.pack(pady=5)
        self.combo_preset = ctk.CTkComboBox(row_preset, values=self.preset_names)
        self.combo_preset.pack(side="left", padx=5)
        
        ctk.CTkButton(row_preset, text="Učitaj", width=60, command=self.load_preset).pack(side="left", padx=2)
        ctk.CTkButton(row_preset, text="Spremi (Za Misiju)", width=60, fg_color="orange", command=self.save_preset).pack(side="left", padx=2)

        self.sliders = []
        self.servo_entries = []
        servo_names = ["Baza", "Rame", "Lakat", "Zglob", "Hvat"]
        default_angles = [135, 164, 40, 100, 20]
        
        self.frame_sliders = ctk.CTkScrollableFrame(col2, height=180)
        self.frame_sliders.pack(fill="both", expand=True, padx=20, pady=5)

        for i in range(5):
            max_angle = 270 if i == 0 else 180
            self.create_servo_control(self.frame_sliders, i, servo_names[i], max_angle, default_angles[i])

    def create_servo_control(self, parent, idx, name, max_angle=180, default_val=90):
        f = ctk.CTkFrame(parent)
        f.pack(fill="x", padx=5, pady=2)
        
        ctk.CTkLabel(f, text=f"{idx}-{name}", width=50).pack(side="left")
        
        entry = ctk.CTkEntry(f, width=50)
        entry.pack(side="right", padx=2)
        
        slider = ctk.CTkSlider(f, from_=0, to=max_angle, number_of_steps=max_angle)
        slider.set(default_val)
        entry.insert(0, str(default_val))
        slider.pack(side="left", fill="x", expand=True, padx=5)
        
        # Callbacks
        def on_slide(val):
            entry.delete(0, "end")
            entry.insert(0, f"{int(val)}")
            self.send_servo(idx, val)
            
        def on_entry(event):
            try:
                val = float(entry.get())
                if 0 <= val <= max_angle:
                    slider.set(val)
                    self.send_servo(idx, val)
            except: pass

        slider.configure(command=on_slide)
        entry.bind("<Return>", on_entry)
        
        self.sliders.append(slider)
        self.servo_entries.append(entry)

    def create_config_row(self, parent, label, default, cmd):
        f = ctk.CTkFrame(parent, fg_color="transparent")
        f.pack(fill="x", pady=2)
        ctk.CTkLabel(f, text=label, width=60, anchor="w").pack(side="left")
        e = ctk.CTkEntry(f, width=60)
        e.pack(side="left", padx=5)
        e.insert(0, default)
        ctk.CTkButton(f, text="SET", width=40, command=cmd).pack(side="right", padx=5)
        return e

    def create_labeled_entry(self, parent, label, default):
        f = ctk.CTkFrame(parent, fg_color="transparent")
        f.pack(fill="x", pady=2)
        ctk.CTkLabel(f, text=label, width=60, anchor="w").pack(side="left")
        e = ctk.CTkEntry(f)
        e.pack(side="right", expand=True, fill="x")
        e.insert(0, default)
        return e

    def send_imp(self):
        try:
            val = float(self.entry_imp.get())
            self.robot.send_command(json.dumps({"cmd": "set_move_cfg", "imp": val}))
            print(f"Sent Imp: {val}")
        except: pass

    def send_deg(self):
        try:
            val = float(self.entry_deg.get())
            self.robot.send_command(json.dumps({"cmd": "set_move_cfg", "deg": val}))
            print(f"Sent Deg: {val}")
        except: pass

    def send_spd(self):
        try:
            val = int(self.entry_spd.get())
            self.robot.send_command(json.dumps({"cmd": "set_move_cfg", "spd": val}))
            print(f"Sent Spd: {val}")
        except: pass

    def send_min(self):
        try:
            val = int(self.entry_min.get())
            self.robot.send_command(json.dumps({"cmd": "set_move_cfg", "min": val}))
            print(f"Sent Min: {val}")
        except: pass

    def send_kp(self):
        try:
            val = float(self.entry_kp.get())
            self.robot.send_command(json.dumps({"cmd": "set_move_cfg", "kp": val}))
            print(f"Sent Kp: {val}")
        except: pass

    def send_move_cfg(self):
        try:
            imp = float(self.entry_imp.get())
            deg = float(self.entry_deg.get())
            spd = int(self.entry_spd.get())
            min_spd = int(self.entry_min.get())
            kp = float(self.entry_kp.get())
            
            payload = {
                "cmd": "set_move_cfg",
                "imp": imp,
                "deg": deg,
                "spd": spd,
                "min": min_spd,
                "kp": kp
            }
            self.robot.send_command(json.dumps(payload))
            messagebox.showinfo("Info", "Parametri kretanja poslani!")
        except Exception as e:
            messagebox.showerror("Error", f"Neispravni podaci: {e}")

    def send_servo(self, ch, val):
        self.robot.send_command(f"SERVO:{ch},{val:.1f}")
        
    # send_color uklonjeno jer više nema kalibracije boja na sučelju

    def start_video_stream(self):
        ip = self.entry_ip.get()
        # Update Wifi Checker IP just in case
        self.wifi_checker.ip = ip 
        # Ensure URL is set (in case IP changed)
        self.video_viewer.url = f"http://{ip}:8080"
        self.video_viewer.stop()
        self.video_viewer.start()

    def stop_calibration_stream(self):
        self.video_viewer.stop()
        self.video_viewer.configure(image=None, text="Stream Stopped")

    def stop_auto_stream(self):
        self.stream_viewer.stop()
        self.stream_viewer.configure(image=None, text="Stream Stopped")

    def update_wifi_status(self, is_online):
         color = "green" if is_online else "red"
         text = f"WiFi Connected ({self.wifi_checker.ip})" if is_online else f"WiFi Disconnected ({self.wifi_checker.ip})"
         
         # Update Calibration Tab
         if hasattr(self, 'lbl_wifi_led'):
             self.lbl_wifi_led.configure(text_color=color)
             self.lbl_wifi_text.configure(text=text)
             
         # Update Auto Tab
         if hasattr(self, 'lbl_auto_wifi_led'):
             self.lbl_auto_wifi_led.configure(text_color=color)
             self.lbl_auto_wifi_text.configure(text=text)

    def setup_manual_tab(self):
        # NEW IMPLEMENTATION OF "Mission Planner"
        self.tab_manual.grid_columnconfigure(0, weight=1) # Sidebar
        self.tab_manual.grid_columnconfigure(1, weight=2) # Inspector
        self.tab_manual.grid_columnconfigure(2, weight=2) # Editor
        self.tab_manual.grid_rowconfigure(0, weight=1)

        # 1. Sidebar: Function Library
        self.frame_sidebar = ctk.CTkFrame(self.tab_manual, width=180, corner_radius=0)
        self.frame_sidebar.grid(row=0, column=0, sticky="nsew", padx=2, pady=2)
        
        ctk.CTkLabel(self.frame_sidebar, text="BIBLIOTEKA", font=("Arial", 16, "bold")).pack(pady=10)
        
        funcs = [
            ("VOZI RAVNO", "straight"),
            ("VOZI (L/D)", "move_dual"),
            ("OKRENI (Spot)", "turn"),
            ("SKRENI (Pivot)", "pivot"),
            ("RUKA", "arm"),
            ("PROVJERI SENZOR", "check_sensor"),
            ("KAMERA MOD", "nicla"),
            ("AKO JE TOČNO", "if_true"),
            ("INAČE", "if_false"),
            ("KRAJ BLOKA", "end_if"),
            ("KALIBRIRAJ IMU", "cal_imu")
        ]
        
        for lbl, cmd in funcs:
            ctk.CTkButton(self.frame_sidebar, text=lbl, command=lambda c=cmd: self.load_inspector(c)).pack(pady=5, padx=10, fill="x")

        # --- STOP BUTTON ---
        ctk.CTkButton(self.frame_sidebar, text="STANI (STOP)", fg_color="red", height=50, font=("Arial", 16, "bold"), command=self.stop_robot_now).pack(pady=(20, 5), padx=10, fill="x")

        # --- RESET BUTTONS ---
        ctk.CTkLabel(self.frame_sidebar, text="--- Reseti ---", font=("Arial", 12)).pack(pady=(20, 5))
        ctk.CTkButton(self.frame_sidebar, text="RESET ENCODER", fg_color="orange", command=self.reset_encoder_cmd).pack(pady=2, padx=10, fill="x")
        ctk.CTkButton(self.frame_sidebar, text="KALIBRIRAJ IMU", fg_color="orange", command=self.calibrate_imu_cmd).pack(pady=2, padx=10, fill="x")
        ctk.CTkButton(self.frame_sidebar, text="RESET KUT (0°)", fg_color="blue", command=self.reset_angle_cmd).pack(pady=2, padx=10, fill="x")

        # --- TELEMETRY BEAUTIFICATION ---
        ctk.CTkLabel(self.frame_sidebar, text="--- Telemetrija ---", font=("Arial", 14, "bold")).pack(pady=(20, 5))
        
        # Grid System for Telemetry
        self.frame_telemetry = ctk.CTkFrame(self.frame_sidebar, fg_color="transparent")
        self.frame_telemetry.pack(fill="x", padx=2)
        
        # Row 1: Distance & Arm
        r1 = ctk.CTkFrame(self.frame_telemetry)
        r1.pack(fill="x", pady=2)
        self.lbl_dist = ctk.CTkLabel(r1, text="Dist: 0 cm", font=("Consolas", 12, "bold"))
        self.lbl_dist.pack(side="left", padx=5)
        self.lbl_arm = ctk.CTkLabel(r1, text="Arm: -", font=("Consolas", 12))
        self.lbl_arm.pack(side="right", padx=5)

        # Row 2: IMU (Gyro & Mag)
        r2 = ctk.CTkFrame(self.frame_telemetry)
        r2.pack(fill="x", pady=2)
        self.lbl_imu_manual = ctk.CTkLabel(r2, text="IMU: Gyro 0.0° | Mag 0.0°", font=("Arial", 13, "bold"), text_color="cyan")
        self.lbl_imu_manual.pack(pady=2)

        # Row 3: Encoders
        r3 = ctk.CTkFrame(self.frame_telemetry)
        r3.pack(fill="x", pady=2)
        self.lbl_enc = ctk.CTkLabel(r3, text="L:0 R:0", font=("Consolas", 11))
        self.lbl_enc.pack(expand=True)
        
        # Row 4: Ultrasonic
        r4 = ctk.CTkFrame(self.frame_telemetry)
        r4.pack(fill="x", pady=2)
        self.lbl_us_fb = ctk.CTkLabel(r4, text="F: - | B: -", font=("Consolas", 11))
        self.lbl_us_fb.pack()
        self.lbl_us_lr = ctk.CTkLabel(r4, text="L: - | R: -", font=("Consolas", 11))
        self.lbl_us_lr.pack()

        # Row 5: Induktivni senzor
        r5 = ctk.CTkFrame(self.frame_telemetry)
        r5.pack(fill="x", pady=2)
        self.lbl_ind = ctk.CTkLabel(r5, text="Senzor: Čisto", font=("Consolas", 13, "bold"), text_color="white")
        self.lbl_ind.pack(pady=2)

        # Row 6: Odredišta (Spremnici)
        r6 = ctk.CTkFrame(self.frame_telemetry)
        r6.pack(fill="x", pady=2)
        self.lbl_dest = ctk.CTkLabel(r6, text="Odr: M:- P:- S:-", font=("Consolas", 11), text_color="yellow")
        self.lbl_dest.pack(pady=2)

        # 2. Inspector
        self.frame_inspector = ctk.CTkFrame(self.tab_manual, corner_radius=0)
        self.frame_inspector.grid(row=0, column=1, sticky="nsew", padx=2, pady=2)
        
        ctk.CTkLabel(self.frame_inspector, text="INSPECTOR", font=("Arial", 16, "bold")).pack(pady=10)

        # --- DNO INSPECTORA: Postavke i Manipulator ---
        self.frame_bottom_inspector = ctk.CTkFrame(self.frame_inspector, height=280)
        self.frame_bottom_inspector.pack(side="bottom", fill="x", padx=5, pady=5)
        
        # Lijevo: Postavke Kretanja
        self.frame_move = ctk.CTkFrame(self.frame_bottom_inspector)
        self.frame_move.pack(side="left", fill="both", expand=True, padx=2, pady=2)
        
        ctk.CTkLabel(self.frame_move, text="Postavke Kretanja", font=("Arial", 12, "bold")).pack(pady=2)
        self.entry_imp = self.create_config_row(self.frame_move, "Imp/cm:", "40", self.send_imp)
        self.entry_deg = self.create_config_row(self.frame_move, "Imp/Deg:", "5.3", self.send_deg)
        self.entry_spd = self.create_config_row(self.frame_move, "Brzina:", "200", self.send_spd)
        self.entry_min = self.create_config_row(self.frame_move, "Min Brz:", "80", self.send_min)
        self.entry_kp  = self.create_config_row(self.frame_move, "Kp:", "2.0", self.send_kp)

        # Manipulator opcije su prebačene na prvi tab (Kalibracija pozicija)

        # Content Inspector (Gornji dio)
        self.inspector_content = ctk.CTkFrame(self.frame_inspector, fg_color="transparent")
        self.inspector_content.pack(fill="both", expand=True, padx=5, pady=5)
        
        self.lbl_insp_info = ctk.CTkLabel(self.inspector_content, text="Odaberi funkciju...", text_color="gray")
        self.lbl_insp_info.pack(pady=20)
        
        self.current_cmd_type = None
        self.entry_params = {}

        # 3. Editor
        self.frame_editor = ctk.CTkFrame(self.tab_manual, corner_radius=0)
        self.frame_editor.grid(row=0, column=2, sticky="nsew", padx=2, pady=2)
        
        ctk.CTkLabel(self.frame_editor, text="EDITOR MISIJE", font=("Arial", 16, "bold")).pack(pady=10)
        
        self.scroll_editor = ctk.CTkScrollableFrame(self.frame_editor, label_text="Redoslijed Izvođenja")
        self.scroll_editor.pack(fill="both", expand=True, padx=5, pady=5)
        
        frame_ed_ctrl = ctk.CTkFrame(self.frame_editor)
        frame_ed_ctrl.pack(fill="x", padx=5, pady=5)
        
        ctk.CTkButton(frame_ed_ctrl, text="EKSPORTIRAJ (misija.txt)", command=self.save_mission, fg_color="green").pack(side="left", fill="x", expand=True, padx=2)
        ctk.CTkButton(frame_ed_ctrl, text="OČISTI SVE", command=self.clear_mission, fg_color="red").pack(side="right", padx=2)

        # Initialize Mission Steps
        self.misija_koraci = []
        self.editing_index = None

        self.CMD_TRANSLATIONS = {
            "straight": "VOZI RAVNO",
            "move_dual": "VOZI (L/D)",
            "turn": "OKRENI (Spot)",
            "pivot": "SKRENI (Pivot)",
            "arm": "RUKA (Preset)",
            "arm_pos": "RUKA (Pozicija)",
            "wait": "ČEKAJ",
            "stop": "STANI",
            "check_sensor": "PROVJERI SENZOR",
            "if_true": "AKO JE TOČNO",
            "if_false": "INAČE",
            "end_if": "KRAJ BLOKA",
            "cal_imu": "KALIBRIRAJ IMU",
            "nicla": "KAMERA MOD"
        }
        
    def load_inspector(self, cmd_type, data=None):
        for widget in self.inspector_content.winfo_children():
            widget.destroy()
            
        self.current_cmd_type = cmd_type
        self.entry_params = {}
        
        ctk.CTkLabel(self.inspector_content, text=f"Funkcija: {cmd_type.upper()}", font=("Arial", 14, "bold"), text_color="cyan").pack(pady=5)
        
        if cmd_type == "straight":
            self.add_param_input("Udaljenost (cm):", "val", data)
            self.add_param_input("Brzina (0-255, 0=Default):", "spd", data)
        elif cmd_type == "move_dual":
            self.add_param_input("Lijevi PWM:", "l", data)
            self.add_param_input("Desni PWM:", "r", data)
            self.add_param_input("Udaljenost (cm, 0=NonStop):", "dist", data)
        elif cmd_type == "turn":
            self.add_param_input("Kut (stupnjevi):", "val", data)
            self.add_param_input("Brzina (0-255, 0=Default):", "spd", data)
        elif cmd_type == "pivot":
            self.add_param_input("Kut (stupnjevi):", "val", data)
            self.add_param_input("Brzina (0-255, 0=Default):", "spd", data)
            ctk.CTkLabel(self.inspector_content, text="Poz = Desno, Neg = Lijevo", font=("Arial", 10)).pack()
        elif cmd_type == "arm":
            ctk.CTkLabel(self.inspector_content, text="Pozicija Ruke (po nazivu):").pack(anchor="w", padx=10)
            self.combo_arm = ctk.CTkOptionMenu(self.inspector_content, values=self.preset_names)
            self.combo_arm.pack(fill="x", padx=10, pady=5)
            if data and "val" in data: self.combo_arm.set(data["val"])
            self.entry_params["val"] = self.combo_arm
        elif cmd_type == "arm_pos":
            angles = data.get("angles", [90,90,90,90,90]) if data else [90,90,90,90,90]
            self.add_param_input("Baza:", "a0", {"a0": angles[0]})
            self.add_param_input("Rame:", "a1", {"a1": angles[1]})
            self.add_param_input("Lakat:", "a2", {"a2": angles[2]})
            self.add_param_input("Zglob:", "a3", {"a3": angles[3]})
            self.add_param_input("Hvataljka:", "a4", {"a4": angles[4]})
        elif cmd_type == "nicla":
            ctk.CTkLabel(self.inspector_content, text="Odaberi Mod Kamere:").pack(anchor="w", padx=10)
            nicla_mods = ["0 - IDLE", "1 - TOF (Mjerenje)", "2 - CESTA (Linija)", "3 - QR (Skaner)"]
            self.combo_nicla = ctk.CTkOptionMenu(self.inspector_content, values=nicla_mods)
            self.combo_nicla.pack(fill="x", padx=10, pady=5)
            if data and "mod" in data: 
                # Pokušaj postaviti stari očitani mod, ali ako je u misiji zapisano samo "c", pretvori u opciju padajuceg izbornika
                val = data["mod"]
                for nm in nicla_mods:
                    if nm.startswith(val) or val in nm:
                        self.combo_nicla.set(nm)
                        break
            self.entry_params["mod"] = self.combo_nicla
        elif cmd_type == "check_sensor":
            ctk.CTkLabel(self.inspector_content, text="Tip senzora:").pack(anchor="w", padx=10)
            self.combo_sens = ctk.CTkOptionMenu(self.inspector_content, values=["induktivni"])
            self.combo_sens.pack(fill="x", padx=10, pady=5)
            if data and "type" in data: self.combo_sens.set(data["type"])
            self.entry_params["type"] = self.combo_sens
        elif cmd_type in ["if_true", "if_false", "end_if"]:
            ctk.CTkLabel(self.inspector_content, text="Kontrolni blok (nema parametara)", text_color="gray").pack(pady=10)
        elif cmd_type == "cal_imu":
            ctk.CTkLabel(self.inspector_content, text="Kalibracija žiroskopa (cca 2 sekunde)", text_color="orange").pack(pady=10)

        btn_text = "SPREMI PROMJENE" if self.editing_index is not None else "IZVRŠI I DODAJ"
        btn_cmd = self.save_edit if self.editing_index is not None else self.execute_and_add
        ctk.CTkButton(self.inspector_content, text=btn_text, command=btn_cmd, height=40, font=("Arial", 14, "bold")).pack(pady=20, fill="x", padx=10)
        
        if self.editing_index is not None:
             ctk.CTkButton(self.inspector_content, text="ODUSTANI", fg_color="gray", command=self.cancel_edit).pack(pady=5, fill="x", padx=10)

    def add_param_input(self, label_text, key, data=None):
        ctk.CTkLabel(self.inspector_content, text=label_text).pack(anchor="w", padx=10)
        entry = ctk.CTkEntry(self.inspector_content)
        entry.pack(fill="x", padx=10, pady=5)
        if data and key in data: return entry.insert(0, str(data[key]))
        self.entry_params[key] = entry

    def get_payload_from_ui(self):
        if not self.current_cmd_type: return None
        payload = {"cmd": self.current_cmd_type}
        try:
            for key, widget in self.entry_params.items():
                if isinstance(widget, ctk.CTkEntry):
                    val_str = widget.get()
                    if key in ["l", "r", "spd"]:
                         try: payload[key] = int(val_str)
                         except: payload[key] = 0
                    elif key == "dist":
                         try: payload[key] = float(val_str)
                         except: payload[key] = 0.0
                    else:
                         try: payload[key] = float(val_str)
                         except: payload[key] = val_str
                elif isinstance(widget, ctk.CTkOptionMenu):
                    payload[key] = widget.get()
            
            # Postprocesiranje
            if self.current_cmd_type == "nicla" and "mod" in payload:
                val = payload["mod"]
                if "0" in val: payload["mod"] = "0"
                elif "1" in val or "TOF" in val: payload["mod"] = "u"
                elif "2" in val or "CESTA" in val: payload["mod"] = "c"
                elif "3" in val or "QR" in val: payload["mod"] = "q"
                
            if self.current_cmd_type == "arm" and "val" in payload:
                # Automatski prebaci 'arm' u 'arm_pos' (čisti kutevi u misiju!)
                preset = payload["val"]
                if preset in self.preset_angles:
                    payload = {"cmd": "arm_pos", "angles": self.preset_angles[preset], "preset_name": preset}
            
            if self.current_cmd_type == "arm_pos" and "a0" in payload:
                angles = [int(payload["a0"]), int(payload["a1"]), int(payload["a2"]), int(payload["a3"]), int(payload["a4"])]
                preset_name = payload.get("preset_name", "CUSTOM")
                payload = {"cmd": "arm_pos", "angles": angles, "preset_name": preset_name}
                
            return payload
        except ValueError:
            messagebox.showerror("Error", "Greska u brojevima!")
            return None

    def execute_and_add(self):
        payload = self.get_payload_from_ui()
        if not payload: return

        # Filtriraj ključeve koji su samo za prikaz (preset_name) - Arduino ih ne koristi
        # ali ih zadrzavamo u lokalnoj memoriji misije za prikaz u editoru
        DISPLAY_ONLY_KEYS = {"preset_name"}
        send_payload = {k: v for k, v in payload.items() if k not in DISPLAY_ONLY_KEYS}
        
        json_str = json.dumps(send_payload)
        self.robot.send_command(json_str) # Send JSON
        
        self.misija_koraci.append(payload)
        self.refresh_editor()

    def save_edit(self):
        if self.editing_index is None: return
        payload = self.get_payload_from_ui()
        if not payload: return
        
        self.misija_koraci[self.editing_index] = payload
        self.editing_index = None
        self.refresh_editor()
        # Reset inspector
        for widget in self.inspector_content.winfo_children(): widget.destroy()
        self.lbl_insp_info = ctk.CTkLabel(self.inspector_content, text="Izmjena Spremljena", text_color="green")
        self.lbl_insp_info.pack(pady=20)

    def cancel_edit(self):
        self.editing_index = None
        self.refresh_editor() # Just to be safe/reset UI state
        for widget in self.inspector_content.winfo_children(): widget.destroy()
        self.lbl_insp_info = ctk.CTkLabel(self.inspector_content, text="Izmjena Otkazana", text_color="gray")
        self.lbl_insp_info.pack(pady=20)

    def edit_step(self, index):
        if 0 <= index < len(self.misija_koraci):
            self.editing_index = index
            step = self.misija_koraci[index]
            self.load_inspector(step["cmd"], step)
            self.refresh_editor() # Highlight current editing step? (Optional, skipping for now)

    def refresh_editor(self):
        for w in self.scroll_editor.winfo_children():
            w.destroy()
        for i, step in enumerate(self.misija_koraci):
            f = ctk.CTkFrame(self.scroll_editor)
            f.pack(fill="x", pady=2)
            
            # Format text
            cmd_key = step.get('cmd', '???')
            cmd_name = self.CMD_TRANSLATIONS.get(cmd_key, cmd_key.upper())
            
            txt = f"{i+1}. {cmd_name}"
            
            # Filter out 'cmd' from parameters
            params = []
            for k, v in step.items():
                if k != "cmd":
                    params.append(f"{k}:{v}")
            
            if params:
                txt += f" ({', '.join(params)})"
            
            lbl = ctk.CTkLabel(f, text=txt, anchor="w", font=("Consolas", 12))
            lbl.pack(side="left", padx=5)
            
            ctk.CTkButton(f, text="X", width=30, fg_color="red", command=lambda idx=i: self.delete_step(idx)).pack(side="right", padx=2)
            ctk.CTkButton(f, text="E", width=30, fg_color="blue", command=lambda idx=i: self.edit_step(idx)).pack(side="right", padx=2)
            
            if i > 0: ctk.CTkButton(f, text="▲", width=30, command=lambda idx=i: self.move_step(idx, -1)).pack(side="right", padx=2)
            if i < len(self.misija_koraci)-1: ctk.CTkButton(f, text="▼", width=30, command=lambda idx=i: self.move_step(idx, 1)).pack(side="right", padx=2)

    def delete_step(self, index):
        if 0 <= index < len(self.misija_koraci):
            self.misija_koraci.pop(index)
            self.refresh_editor()

    def move_step(self, index, direction):
        new_idx = index + direction
        if 0 <= new_idx < len(self.misija_koraci):
            self.misija_koraci[index], self.misija_koraci[new_idx] = self.misija_koraci[new_idx], self.misija_koraci[index]
            self.refresh_editor()
            
    def clear_mission(self):
        self.misija_koraci = []
        self.refresh_editor()

    def stop_robot_now(self):
        print("STOPPING ROBOT!")
        self.robot.send_command(json.dumps({"cmd": "stop"}))
        # Also stop local mission if running
        if self.mission_running:
            self.stop_mission()

    def update_telemetry(self, data):
        self.latest_telemetry = data

    def ui_updater(self):
        if hasattr(self, 'latest_telemetry') and self.latest_telemetry:
            d = self.latest_telemetry
            # Update Labels
            # Update Labels
            self.lbl_dist.configure(text=f"Dist: {d.get('cm', '0')} cm")
            self.lbl_arm.configure(text=f"Arm: {d.get('arm', '-')}")
            if hasattr(self, 'lbl_imu_manual'):
                self.lbl_imu_manual.configure(text=f"IMU: {d.get('gyro', '0.0')}° | {d.get('gz', '0.0')}°/s")
            if hasattr(self, 'lbl_tof') and 'tof' in d:
                self.lbl_tof.configure(text=f"TOF: {d['tof']} mm")
            if hasattr(self, 'lbl_nicla_ip') and 'ip' in d and d['ip'] != "0.0.0.0":
                self.lbl_nicla_ip.configure(text=f"Nicla: {d['ip']}")
            
            self.lbl_enc.configure(text=f"Enc: L={d['pL']} R={d['pR']}")
            self.lbl_us_fb.configure(text=f"F: {d['usF']} | B: {d['usB']}")
            self.lbl_us_lr.configure(text=f"L: {d['usL']} | R: {d['usR']}")
            
            # Induktivni senzor UI update
            if hasattr(self, 'lbl_ind') and 'ind' in d:
                if d['ind'] == '1':
                    self.lbl_ind.configure(text="Senzor: LIMENKA", text_color="#00FF00") # Zelena
                else:
                    self.lbl_ind.configure(text="Senzor: Čisto", text_color="white")
                    
            # Odredista
            if hasattr(self, 'lbl_dest') and 'destM' in d:
                self.lbl_dest.configure(text=f"Odr: M:{d['destM']} P:{d['destP']} S:{d['destS']}")
            
            # Auto Tab Updates
            if hasattr(self, 'lbl_auto_dist'):
                self.lbl_auto_dist.configure(text=f"Distance (TOF): {d.get('tof', '0')} mm | Enc: {d.get('pL', '')}/{d.get('pR', '')}")
            if hasattr(self, 'lbl_auto_imu'):
                self.lbl_auto_imu.configure(text=f"IMU: Gyro {d.get('gyro', '0.0')}° | Raw {d.get('gz', '0.0')}°/s")
            if hasattr(self, 'lbl_auto_sensors'):
                self.lbl_auto_sensors.configure(text=f"US: F={d.get('usF')} B={d.get('usB')} L={d.get('usL')} R={d.get('usR')} | I={d.get('ind')}")
            
        self.after(50, self.ui_updater)

    def exec_record_arm(self):
        preset = self.man_preset_combo.get()
        # 1. Execute
        try:
             idx = self.preset_names.index(preset)
             self.robot.send_command(f"LOAD_PRESET:{idx}")
             # 2. Record
             self.log_mission_step(f"ARM:{preset}")
        except: pass

    def take_snapshot(self):
        # Fetch single frame
        ip = self.entry_ip.get()
        url = f"http://{ip}:8080" # Root returns MJPEG stream usually...
        # Nicla script sends MJPEG. We can just open, read one frame, close.
        threading.Thread(target=self._snapshot_thread, args=(url,), daemon=True).start()
        
    def _snapshot_thread(self, url):
        try:
            self.lbl_snapshot_status.configure(text="Fetching...")
            stream = requests.get(url, stream=True, timeout=2)
            if stream.status_code == 200:
                bytes_data = bytes()
                for chunk in stream.iter_content(chunk_size=1024):
                    bytes_data += chunk
                    a = bytes_data.find(b'\xff\xd8')
                    b = bytes_data.find(b'\xff\xd9')
                    if a != -1 and b != -1:
                        jpg = bytes_data[a:b+2]
                        # Show in popup
                        self.show_image_popup(jpg)
                        break
            self.lbl_snapshot_status.configure(text="Done")
        except Exception as e:
            self.lbl_snapshot_status.configure(text="Error")
            print(e)
            
    def log_mission_step(self, text):
        self.waypoints_list.configure(state="normal")
        self.waypoints_list.insert("end", text + "\n")
        self.waypoints_list.see("end")
        self.waypoints_list.configure(state="disabled")

    def setup_auto_tab(self):
        frame = self.tab_auto
        
        # Layout: Left (Stream/Telemetry) vs Right (Mission Control)
        frame.columnconfigure(0, weight=1)
        frame.columnconfigure(1, weight=1)
        
        left_col = ctk.CTkFrame(frame)
        left_col.grid(row=0, column=0, sticky="nsew", padx=5, pady=5)
        
        right_col = ctk.CTkFrame(frame)
        right_col.grid(row=0, column=1, sticky="nsew", padx=5, pady=5)
        
        # --- LEFT: Stream & Telemetry ---
        ctk.CTkLabel(left_col, text="Kamera Uživo", font=("Arial", 16, "bold")).pack(pady=5)
        
        # Nicla WiFi Status (Auto Tab)
        self.wifi_auto_frame = ctk.CTkFrame(left_col, fg_color="transparent")
        self.wifi_auto_frame.pack(pady=2)
        self.lbl_auto_wifi_led = ctk.CTkLabel(self.wifi_auto_frame, text="●", font=("Arial", 24), text_color="red")
        self.lbl_auto_wifi_led.pack(side="left", padx=2)
        self.lbl_auto_wifi_text = ctk.CTkLabel(self.wifi_auto_frame, text="WiFi Disconnected")
        self.lbl_auto_wifi_text.pack(side="left", padx=2)

        # MJPEG Viewer
        self.stream_viewer = MJPEGViewer(left_col, url=f"http://{DEFAULT_IP}:8080", width=400, height=300)
        self.stream_viewer.pack(pady=5)
        
        # Auto Stream Buttons
        btn_row_auto = ctk.CTkFrame(left_col, fg_color="transparent")
        btn_row_auto.pack(pady=2)
        ctk.CTkButton(btn_row_auto, text="Osvježi Stream (IP)", command=self.refresh_stream).pack(side="left", padx=2)
        ctk.CTkButton(btn_row_auto, text="Prekini Stream", fg_color="red", command=self.stop_auto_stream).pack(side="left", padx=2)

        # Kamera Modovi - NOVO!
        ctk.CTkLabel(left_col, text="Kamera Modovi").pack(pady=(10, 0))
        btn_row_cam_mods = ctk.CTkFrame(left_col, fg_color="transparent")
        btn_row_cam_mods.pack(pady=2)
        
        ctk.CTkButton(btn_row_cam_mods, text="IDLE", width=60, fg_color="orange", command=lambda: self.robot.send_command("NICLA:0")).pack(side="left", padx=2)
        ctk.CTkButton(btn_row_cam_mods, text="TOF", width=60, command=lambda: self.robot.send_command("NICLA:u")).pack(side="left", padx=2)
        ctk.CTkButton(btn_row_cam_mods, text="CESTA", width=60, command=lambda: self.robot.send_command("NICLA:c")).pack(side="left", padx=2)
        ctk.CTkButton(btn_row_cam_mods, text="QR", width=60, command=lambda: self.robot.send_command("NICLA:q")).pack(side="left", padx=2)

        ctk.CTkLabel(left_col, text="--- Telemetrija ---").pack(pady=10)
        self.lbl_auto_dist = ctk.CTkLabel(left_col, text="Distance (TOF): 0 mm", font=("Arial", 14))
        self.lbl_auto_dist.pack()
        
        self.lbl_auto_imu = ctk.CTkLabel(left_col, text="IMU: Gyro 0.0° | Mag 0.0°", font=("Arial", 14, "bold"), text_color="cyan")
        self.lbl_auto_imu.pack(pady=5)
        
        self.lbl_auto_sensors = ctk.CTkLabel(left_col, text="Sensors: -")
        self.lbl_auto_sensors.pack()
        self.lbl_auto_status = ctk.CTkLabel(left_col, text="Robot Status: IDLE", text_color="orange")
        self.lbl_auto_status.pack(pady=5)

        # --- RIGHT: Mission Control ---
        ctk.CTkLabel(right_col, text="Učitana Misija", font=("Arial", 16)).pack(pady=5)
        
        self.log_box = ctk.CTkTextbox(right_col, width=400, height=300)
        self.log_box.pack(pady=5, fill="both", expand=True)
        
        btn_row = ctk.CTkFrame(right_col, fg_color="transparent")
        btn_row.pack(pady=10)
        
        ctk.CTkButton(btn_row, text="Učitaj (misija.txt)", command=self.load_mission).pack(side="left", padx=5)
        ctk.CTkButton(btn_row, text="POKRENI MISIJU", fg_color="green", command=self.start_mission).pack(side="left", padx=5)
        ctk.CTkButton(btn_row, text="STOP", fg_color="red", command=self.stop_mission).pack(side="left", padx=5)

        self.mission_running = False

    def refresh_stream(self):
         ip = self.entry_ip.get()
         self.stream_viewer.url = f"http://{ip}:8080"
         self.stream_viewer.stop()
         self.stream_viewer.start()

    def start_mission(self):
        if self.mission_running: return
        self.mission_running = True
        threading.Thread(target=self._wait_start_then_run, daemon=True).start()

    def _wait_start_then_run(self):
        self.lbl_auto_status.configure(text="⏳ Čekam START gumb na robotu...", text_color="yellow")
        # Pošalji wait_start i čekaj DONE (fizički gumb pritisnut) - timeout 60s
        resp = self.robot.send_command_and_wait(json.dumps({"cmd": "wait_start"}), timeout=60.0)
        if not self.mission_running:
            return  # Korisnik pritisnuo STOP u međuvremenu
        if resp and resp.get("status") == "DONE":
            self.lbl_auto_status.configure(text="▶ START! Izvršavam misiju...", text_color="lime")
            self.run_mission_thread()
        else:
            self.mission_running = False
            self.lbl_auto_status.configure(text="⚠ Timeout - START nije pritisnut (60s)", text_color="red")

    def stop_mission(self):
        self.mission_running = False
        self.robot.send_command(json.dumps({"cmd": "stop"}))
        self.lbl_auto_status.configure(text="STATUS: STOPPED", text_color="red")

    def run_mission_thread(self):
        lines = self.log_box.get("1.0", "end").splitlines()
        mission = []
        for line in lines:
            line = line.strip()
            if not line or line.startswith("#"): continue
            try:
                mission.append(json.loads(line))
            except:
                # Ako nije JSON, pospremi kao raw string objekt
                mission.append({"cmd": "raw", "val": line})

        last_condition_met = True
        i = 0
        while i < len(mission):
            if not self.mission_running: break
            step = mission[i]
            cmd_type = step.get("cmd")
            
            self.lbl_auto_status.configure(text=f"Izvršavam: {cmd_type}")

            if cmd_type == "check_sensor":
                # Arduino handler se zove "read_sensor", ne "check_sensor" - prepisujemo cmd
                send_check = dict(step)
                send_check["cmd"] = "read_sensor"
                resp = self.robot.send_command_and_wait(json.dumps(send_check))
                last_condition_met = (resp and resp.get("status") == "1")
                i += 1
                continue
            
            if cmd_type == "if_true":
                if not last_condition_met:
                    # Preskoči do if_false ili end_if
                    while i < len(mission) - 1:
                        i += 1
                        next_cmd = mission[i].get("cmd")
                        if next_cmd == "if_false" or next_cmd == "end_if":
                            break
                    continue
                i += 1
                continue

            if cmd_type == "if_false":
                if last_condition_met:
                    # Preskoči kompletan else blok jer je if bio točan
                    while i < len(mission) - 1:
                        i += 1
                        if mission[i].get("cmd") == "end_if":
                            break
                    continue
                i += 1
                continue
            
            if cmd_type == "end_if":
                i += 1
                continue

            # Ključevi koji su samo za prikaz - ne šalju se Arduinu
            DISPLAY_ONLY_KEYS = {"preset_name"}
            send_step = {k: v for k, v in step.items() if k not in DISPLAY_ONLY_KEYS}
            
            # Standardne komande kretanja, ruke i kamere
            if cmd_type == "wait":
                ms = int(step.get("val", 0))
                time.sleep(ms / 1000.0)
            elif cmd_type in ["straight", "turn", "pivot", "move_dual", "cal_imu"]:
                self.robot.send_command_and_wait(json.dumps(send_step), timeout=20.0)
            elif cmd_type in ["arm", "arm_pos"]:
                self.robot.send_command(json.dumps(send_step))
                time.sleep(3.0)
            elif cmd_type == "nicla":
                self.robot.send_command(json.dumps(send_step))
                time.sleep(0.5)
            elif cmd_type == "raw":
                # Stara komanda (npr. "ARM:1" ili "NICLA:q")
                self.robot.send_command(step["val"])
                time.sleep(0.5)
            else:
                self.robot.send_command(json.dumps(send_step))
            
            i += 1

        self.mission_running = False
        self.lbl_auto_status.configure(text="STATUS: GOTOVO", text_color="blue")

    def create_input(self, parent, label):
        f = ctk.CTkFrame(parent, fg_color="transparent")
        f.pack(fill="x", padx=10, pady=2)
        ctk.CTkLabel(f, text=label, width=30).pack(side="left")
        e = ctk.CTkEntry(f)
        e.pack(side="right", expand=True, fill="x")
        return e

    def save_config(self):
        # 1. PID
        kp = self.entry_kp.get()
        ki = self.entry_ki.get()
        kd = self.entry_kd.get()
        self.robot.send_command(f"PID:{kp},{ki},{kd}")
        time.sleep(0.1)
        
        # 2. Motor
        puls = self.entry_pulses.get()
        # spd = self.entry_speed.get() # Speed not currently used in set_motor/postaviKonfigKretanja
        # self.robot.send_command(f"SET_MOTOR:{puls},{spd}") # OLD
        
        try:
            val = float(puls)
            payload = {"cmd": "set_motor", "val": val}
            self.robot.send_command(json.dumps(payload))
        except:
            messagebox.showerror("Error", "Invalid Pulse Value")

    def save_preset(self):
        try:
            kutevi = [int(s.get()) for s in self.sliders]
            # Uključi bazno ime preseta kao napomenu u misiji kod custom dodavanja
            step = {"cmd": "arm_pos", "angles": kutevi, "preset_name": self.combo_preset.get() + "-custom"}
            self.misija_koraci.append(step)
            self.refresh_editor()
            messagebox.showinfo("Spremljeno", f"Trenutni kutevi dodani u misiju kao custom pozicija:\n{kutevi}")
        except Exception as e:
            messagebox.showerror("Greška", f"Nije moguće dodati u misiju: {e}")

    def load_preset(self):
        name = self.combo_preset.get()
        if name in self.preset_angles:
            kutevi = self.preset_angles[name]
            # Pošalji robotu
            payload = {"cmd": "arm_pos", "angles": kutevi}
            self.robot.send_command(json.dumps(payload))
            
            # Ažuriraj lokalne slidere automatski
            for i in range(5):
                self.sliders[i].set(kutevi[i])
                self.servo_entries[i].delete(0, "end")
                self.servo_entries[i].insert(0, str(kutevi[i]))
                
        else:
            messagebox.showerror("Error", "Nepoznata pozicija")

    def on_key_press(self, event):
        if self.tabview.get() != "Učenje (Manual)": return
        
        # Ignoriraj ako korisnik tipka u neki Entry widget (npr. upisuje brzinu ili kut)
        focused = self.focus_get()
        if isinstance(focused, ctk.CTkEntry) or isinstance(focused, tk.Entry):
            return
        
        key = event.keysym
        speed = 100
        cmd = ""
        
        if key == 'KP_Up': cmd = f"MAN:FWD,{speed}"
        elif key == 'KP_Down': cmd = f"MAN:BCK,{speed}"
        elif key == 'KP_Left': cmd = f"MAN:LFT,{speed}"
        elif key == 'KP_Right': cmd = f"MAN:RGT,{speed}"
            
        if cmd: self.robot.send_command(cmd)

    def save_mission(self):
        try:
            filename = filedialog.asksaveasfilename(defaultextension=".txt", initialfile="misija.txt")
            if not filename: return
            
            with open(filename, "w", encoding="utf-8") as f:
                for step in self.misija_koraci:
                    f.write(json.dumps(step) + "\n")
            messagebox.showinfo("Saved", f"Misija spremljena u {filename}")
        except Exception as e:
            messagebox.showerror("Error", f"Could not save: {e}")

    def reset_encoder_cmd(self):
        self.robot.send_command('{"cmd": "reset_enc"}')
        
    def calibrate_imu_cmd(self):
        if messagebox.askyesno("Kalibracija", "Robot mora potpuno mirovati 2 sekunde. Nastaviti?"):
            self.robot.send_command(json.dumps({"cmd": "cal_imu"}))

    def reset_angle_cmd(self):
        self.robot.send_command(json.dumps({"cmd": "reset_imu"}))

    def load_mission(self):
        try:
            filename = filedialog.askopenfilename(defaultextension=".txt", filetypes=[("Text Files", "*.txt"), ("All Files", "*.*")])
            if not filename: return
            
            with open(filename, "r", encoding="utf-8-sig") as f:
                content = f.read()
                self.log_box.delete("1.0", "end")
                self.log_box.insert("end", content)
            
            print(f"Loaded mission from {filename}")
            self.lbl_auto_status.configure(text="Misija učitana - šaljem ruku u PARKING...", text_color="orange")
            
            # Odgoditi slanje 300ms da se filedialog prozor potpuno zatvori
            # inače Tkinter fokus interferira s BLE asyncio loopom (bug pri 2. učitavanju)
            parking_cmd = json.dumps({"cmd": "arm_pos", "angles": [135, 164, 40, 100, 20]})
            self.after(300, lambda: self._send_parking_and_status(parking_cmd))
            
        except Exception as e:
            messagebox.showerror("Error", f"Could not load: {e}")

    def _send_parking_and_status(self, parking_cmd):
        # Najprije pošalji stop da prekinemo eventualni wait_start blokrajući loop na Arduinu
        self.robot.send_command(json.dumps({"cmd": "stop"}))
        import time; time.sleep(0.2)  # Mali razmak da robot obradi stop
        self.robot.send_command(parking_cmd)
        self.lbl_auto_status.configure(text="✅ Ruka u PARKING - pritisni POKRENI MISIJU.", text_color="orange")
        print("Arm → PARKING (čeka START)")


    def on_ble_disconnect(self):
        self.after(0, self._handle_disconnect_ui)

    def _handle_disconnect_ui(self):
        self.btn_connect.configure(text="Connect BLE (HC-02)", fg_color=["#3B8ED0", "#1F6AA5"]) 
        messagebox.showwarning("Veza Prekinuta", "Robot je odspojen!")

if __name__ == "__main__":
    app = DashboardApp()
    app.mainloop()
