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
        self.on_telemetry_callback = None
        self.rx_buffer = ""

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
        self.client = BleakClient(target.address)
        
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

    def notification_handler(self, sender, data):
        try:
            chunk = data.decode('utf-8', errors='ignore')
            self.rx_buffer += chunk
            
            if "\n" in self.rx_buffer:
                lines = self.rx_buffer.split("\n")
                
                for line in lines[:-1]:
                    line = line.strip()
                    if line.startswith("STATUS:"):
                        # STATUS:cm,pL,pR,armIdx,usF,usB,usL,usR,ind
                        parts = line.split(":")[1].split(",")
                        if len(parts) >= 9:
                            data_dict = {
                                "cm": parts[0], "pL": parts[1], "pR": parts[2],
                                "arm": parts[3],
                                "usF": parts[4], "usB": parts[5], "usL": parts[6], "usR": parts[7],
                                "ind": parts[8]
                            }
                            if self.on_telemetry_callback:
                                self.on_telemetry_callback(data_dict)
                    elif line and not line.startswith("DEBUG"): 
                        print(f"RX: {line}")
                            
                self.rx_buffer = lines[-1]
                
        except Exception as e:
            print(f"RX Error: {e}")

    def send_command(self, cmd):
        if self.connected and self.client:
             asyncio.run_coroutine_threadsafe(self.write_ble(cmd), self.loop)

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
        self.entry_ip.grid(row=3, column=0, padx=20, pady=(0, 20))
        self.entry_ip.insert(0, DEFAULT_IP)
        
        # Tabs
        self.tabview = ctk.CTkTabview(self)
        self.tabview.grid(row=0, column=1, padx=20, pady=20, sticky="nsew")
        
        self.tab_calib = self.tabview.add("Kalibracija")
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
        # 3 Columns
        frame = self.tab_calib
        col1 = ctk.CTkFrame(frame)
        col1.pack(side="left", fill="both", expand=True, padx=5, pady=5)
        col2 = ctk.CTkFrame(frame)
        col2.pack(side="left", fill="both", expand=True, padx=5, pady=5)
        col3 = ctk.CTkFrame(frame)
        col3.pack(side="left", fill="both", expand=True, padx=5, pady=5)
        
        # --- COLUMN 1: LOGIKA & MOTORI ---
        ctk.CTkLabel(col1, text="PID & Motori", font=("Arial", 16, "bold")).pack(pady=5)
        
        # PID
        self.entry_kp = self.create_input(col1, "Kp:")
        self.entry_ki = self.create_input(col1, "Ki:")
        self.entry_kd = self.create_input(col1, "Kd:")
        
        ctk.CTkLabel(col1, text="Motori Config", font=("Arial", 12, "bold")).pack(pady=(10,5))
        self.entry_pulses = self.create_input(col1, "Pulse/cm:")
        self.entry_pulses.insert(0, "40.0")
        self.entry_speed = self.create_input(col1, "Brzina:")
        self.entry_speed.insert(0, "100")
        
        ctk.CTkButton(col1, text="Spremi Config (PID/Motor)", command=self.save_config).pack(pady=10)

        # --- COLUMN 2: VIZIJA ---
        ctk.CTkLabel(col2, text="Kamera & Boje", font=("Arial", 16, "bold")).pack(pady=5)
        
        # Nicla WiFi Status (Calibration Tab)
        self.wifi_status_frame = ctk.CTkFrame(col2, fg_color="transparent")
        self.wifi_status_frame.pack(pady=2)
        self.lbl_wifi_led = ctk.CTkLabel(self.wifi_status_frame, text="●", font=("Arial", 24), text_color="red")
        self.lbl_wifi_led.pack(side="left", padx=2)
        self.lbl_wifi_text = ctk.CTkLabel(self.wifi_status_frame, text=f"WiFi Disconnected ({DEFAULT_IP})")
        self.lbl_wifi_text.pack(side="left", padx=2)

        # Video Stream 
        self.video_viewer = MJPEGViewer(col2, "", width=320, height=240, fg_color="black")
        self.video_viewer.pack(pady=5)
        # Button Row for Stream
        btn_row_calib = ctk.CTkFrame(col2, fg_color="transparent")
        btn_row_calib.pack(pady=5)
        ctk.CTkButton(btn_row_calib, text="Start Stream", command=self.start_video_stream).pack(side="left", padx=2)
        ctk.CTkButton(btn_row_calib, text="Prekini Stream", fg_color="red", command=self.stop_calibration_stream).pack(side="left", padx=2)
        
        # Color Sliders
        ctk.CTkLabel(col2, text="Kalibracija Boja").pack()
        self.combo_obj = ctk.CTkComboBox(col2, values=["BOCA", "LIMENKA", "SPUZVA"])
        self.combo_obj.pack(pady=5)
        
        # LAB Inputs
        self.lab_vars = {}
        lab_labels = ["L Min", "L Max", "A Min", "A Max", "B Min", "B Max"]
        lab_defaults = [30, 100, -64, -8, -32, 32] 
        
        self.lab_frame = ctk.CTkFrame(col2)
        self.lab_frame.pack(pady=5)
        
        for i, label in enumerate(lab_labels):
            lbl = ctk.CTkLabel(self.lab_frame, text=label, width=40)
            lbl.grid(row=i//2, column=(i%2)*2, padx=2)
            entry = ctk.CTkEntry(self.lab_frame, width=50)
            entry.grid(row=i//2, column=(i%2)*2+1, padx=2)
            entry.insert(0, str(lab_defaults[i]))
            self.lab_vars[label] = entry
            
        ctk.CTkButton(col2, text="Pošalji Boje (SET)", command=self.send_color).pack(pady=5)
        
        # --- COLUMN 3: ROBOTSKA RUKA ---
        ctk.CTkLabel(col3, text="Manipulator", font=("Arial", 16, "bold")).pack(pady=5)
        
        # Presets
        self.preset_names = [
            "Parkiraj", "Voznja", "Uzmi_Boca", "Uzmi_Limenka", "Uzmi_Spuzva",
            "Spremi_1", "Spremi_2", "Spremi_3",
            "Iz_Sprem_1", "Iz_Sprem_2", "Iz_Sprem_3",
            "Dostava_1", "Dostava_2", "Dostava_3", "Extra"
        ]
        self.combo_preset = ctk.CTkComboBox(col3, values=self.preset_names)
        self.combo_preset.pack(pady=5)
        
        row_preset = ctk.CTkFrame(col3)
        row_preset.pack(pady=5)
        ctk.CTkButton(row_preset, text="Učitaj Preset", width=80, command=self.load_preset).pack(side="left", padx=5)
        ctk.CTkButton(row_preset, text="Spremi Preset", width=80, fg_color="orange", command=self.save_preset).pack(side="left", padx=5)
        
        # Sliders for 6 Servos
        self.sliders = []
        self.servo_entries = [] # To update text on preset load
        servo_names = ["Baza", "Rame", "Lakat", "Zglob", "Rot", "Hvat"]
        
        for i in range(6):
            max_angle = 360 if i == 0 else 180
            self.create_servo_control(col3, i, servo_names[i], max_angle)

    def create_servo_control(self, parent, idx, name, max_angle=180):
        f = ctk.CTkFrame(parent)
        f.pack(fill="x", padx=5, pady=2)
        
        ctk.CTkLabel(f, text=f"{idx}-{name}", width=50).pack(side="left")
        
        entry = ctk.CTkEntry(f, width=50)
        entry.pack(side="right", padx=2)
        
        slider = ctk.CTkSlider(f, from_=0, to=max_angle, number_of_steps=max_angle)
        slider.set(90)
        entry.insert(0, "90")
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

    def send_servo(self, ch, val):
        self.robot.send_command(f"SERVO:{ch},{val:.1f}")
        
    def send_color(self):
        obj = self.combo_obj.get()
        vals = []
        labels = ["L Min", "L Max", "A Min", "A Max", "B Min", "B Max"]
        try:
            for l in labels:
                vals.append(self.lab_vars[l].get())
            cmd = f"SET:{obj},{','.join(vals)}"
            print(f"Sending Color Config: {cmd}")
            self.robot.send_command(cmd)
        except Exception as e:
            messagebox.showerror("Error", f"Invalid Values: {e}")

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
            ("RUKA", "arm")
        ]
        
        for lbl, cmd in funcs:
            ctk.CTkButton(self.frame_sidebar, text=lbl, command=lambda c=cmd: self.load_inspector(c)).pack(pady=5, padx=10, fill="x")

        # 2. Inspector
        self.frame_inspector = ctk.CTkFrame(self.tab_manual, corner_radius=0)
        self.frame_inspector.grid(row=0, column=1, sticky="nsew", padx=2, pady=2)
        
        ctk.CTkLabel(self.frame_inspector, text="INSPECTOR", font=("Arial", 16, "bold")).pack(pady=10)
        
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
        
    def load_inspector(self, cmd_type):
        for widget in self.inspector_content.winfo_children():
            widget.destroy()
            
        self.current_cmd_type = cmd_type
        self.entry_params = {}
        
        ctk.CTkLabel(self.inspector_content, text=f"Funkcija: {cmd_type.upper()}", font=("Arial", 14, "bold"), text_color="cyan").pack(pady=5)
        
        if cmd_type == "straight":
            self.add_param_input("Udaljenost (cm):", "val")
        elif cmd_type == "move_dual":
            self.add_param_input("Lijevi PWM:", "l")
            self.add_param_input("Desni PWM:", "r")
            self.add_param_input("Udaljenost (cm):", "dist")
        elif cmd_type == "turn":
            self.add_param_input("Kut (stupnjevi):", "val")
        elif cmd_type == "pivot":
            self.add_param_input("Kut (stupnjevi):", "val")
            ctk.CTkLabel(self.inspector_content, text="Poz = Desno, Neg = Lijevo", font=("Arial", 10)).pack()
        elif cmd_type == "arm":
            ctk.CTkLabel(self.inspector_content, text="Pozicija Ruke:").pack(anchor="w", padx=10)
            self.combo_arm = ctk.CTkOptionMenu(self.inspector_content, values=[
                "BOCA", "KROV_1", "ODLAGANJE_D1", "ODLAGANJE_D2", "ODLAGANJE_D3", "HOME", "DIZANJE_SIGURNO"
            ])
            self.combo_arm.pack(fill="x", padx=10, pady=5)
            self.entry_params["val"] = self.combo_arm

        ctk.CTkButton(self.inspector_content, text="IZVRŠI I DODAJ", command=self.execute_and_add, height=40, font=("Arial", 14, "bold")).pack(pady=20, fill="x", padx=10)

    def add_param_input(self, label_text, key):
        ctk.CTkLabel(self.inspector_content, text=label_text).pack(anchor="w", padx=10)
        entry = ctk.CTkEntry(self.inspector_content)
        entry.pack(fill="x", padx=10, pady=5)
        self.entry_params[key] = entry

    def execute_and_add(self):
        if not self.current_cmd_type: return
        payload = {"cmd": self.current_cmd_type}
        
        try:
            for key, widget in self.entry_params.items():
                if isinstance(widget, ctk.CTkEntry):
                    val_str = widget.get()
                    if key in ["l", "r"]:
                         payload[key] = int(val_str)
                    else:
                         try: payload[key] = float(val_str)
                         except: payload[key] = val_str
                elif isinstance(widget, ctk.CTkOptionMenu):
                    payload[key] = widget.get()
        except ValueError:
            messagebox.showerror("Error", "Greska u brojevima!")
            return

        json_str = json.dumps(payload)
        self.robot.send_command(json_str) # Send JSON
        
        self.misija_koraci.append(payload)
        self.refresh_editor()

    def refresh_editor(self):
        for w in self.scroll_editor.winfo_children():
            w.destroy()
        for i, step in enumerate(self.misija_koraci):
            f = ctk.CTkFrame(self.scroll_editor)
            f.pack(fill="x", pady=2)
            lbl = ctk.CTkLabel(f, text=f"{i+1}. {step['cmd']}", anchor="w", font=("Consolas", 12))
            lbl.pack(side="left", padx=5)
            ctk.CTkButton(f, text="X", width=30, fg_color="red", command=lambda idx=i: self.delete_step(idx)).pack(side="right", padx=5)
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

    def update_telemetry(self, data):
        self.latest_telemetry = data

    def ui_updater(self):
        if hasattr(self, 'latest_telemetry') and self.latest_telemetry:
            d = self.latest_telemetry
            # Update Labels
            self.lbl_dist.configure(text=f"Distance: {d['cm']} cm")
            self.lbl_enc.configure(text=f"Enc: L={d['pL']} R={d['pR']}")
            self.lbl_arm.configure(text=f"Arm Preset: {d['arm']}")
            self.lbl_us_fb.configure(text=f"Front: {d['usF']} | Back: {d['usB']}")
            self.lbl_us_lr.configure(text=f"Left: {d['usL']} | Right: {d['usR']}")
            
            # Auto Tab Updates
            if hasattr(self, 'lbl_auto_dist'):
                self.lbl_auto_dist.configure(text=f"Distance: {d['cm']} cm | Enc: {d['pL']}/{d['pR']}")
                self.lbl_auto_sensors.configure(text=f"US: F={d['usF']} B={d['usB']} L={d['usL']} R={d['usR']} | I={d['ind']}")
            
        self.after(100, self.ui_updater)

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

        ctk.CTkLabel(left_col, text="--- Telemetrija ---").pack(pady=10)
        self.lbl_auto_dist = ctk.CTkLabel(left_col, text="Distance: 0.0 cm")
        self.lbl_auto_dist.pack()
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
        threading.Thread(target=self.run_mission_thread, daemon=True).start()

    def stop_mission(self):
        self.mission_running = False
        self.robot.send_command("MAN:STOP") # or STANI
        self.lbl_auto_status.configure(text="STATUS: STOPPED", text_color="red")

    def run_mission_thread(self):
        self.lbl_auto_status.configure(text="STATUS: RUNNING", text_color="green")
        lines = self.log_box.get("1.0", "end").splitlines()
        
        for i, line in enumerate(lines):
            if not self.mission_running: break
            line = line.strip()
            if not line or line.startswith("---") or line.startswith("#"): continue
            
            print(f"Executing: {line}")
            # Use 'after' if we want detailed UI update, but thread safe way often required
            # For status label, we risk it or queue it. CTK usually handles text config okay from threads.
            self.lbl_auto_status.configure(text=f"Exec: {line}")
            
            parts = line.split(":") 
            cmd_type = parts[0]
            
            if cmd_type == "MOVE":
                target = int(float(parts[1]))
                cmd = f"MOVE:{target}" 
                self.robot.send_command(cmd)
                
                # Wait logic
                time.sleep(0.5) 
                start_time = time.time()
                while self.mission_running:
                    if time.time() - start_time > 10: break 
                    try:
                        curr = float(self.latest_telemetry.get('cm', 0))
                        if abs(curr) >= abs(target) - 2:
                            break
                    except: pass
                    time.sleep(0.1)
                time.sleep(0.5) 

            elif cmd_type == "ARM":
                preset_name = parts[1]
                try:
                    idx = self.preset_names.index(preset_name)
                    self.robot.send_command(f"LOAD_PRESET:{idx}")
                    time.sleep(2.0)
                except:
                    print(f"Unknown Preset: {preset_name}")
            
            elif cmd_type == "WAIT":
                ms = int(parts[1])
                time.sleep(ms / 1000.0)

        self.mission_running = False
        self.lbl_auto_status.configure(text="STATUS: DONE", text_color="blue")

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
        spd = self.entry_speed.get()
        self.robot.send_command(f"SET_MOTOR:{puls},{spd}")

    def save_preset(self):
        name = self.combo_preset.get()
        try:
             idx = self.preset_names.index(name)
             self.robot.send_command(f"SAVE_PRESET:{idx}")
        except ValueError:
             messagebox.showerror("Error", "Unknown Preset Name")

    def load_preset(self):
        name = self.combo_preset.get()
        try:
             idx = self.preset_names.index(name)
             self.robot.send_command(f"LOAD_PRESET:{idx}")
        except ValueError:
             messagebox.showerror("Error", "Unknown Preset Name")

    def on_key_press(self, event):
        if self.tabview.get() != "Učenje (Manual)": return
        
        key = event.keysym
        speed = 100
        cmd = ""
        
        if key == 'KP_Up' or key == '8': cmd = f"MAN:FWD,{speed}"
        elif key == 'KP_Down' or key == '2': cmd = f"MAN:BCK,{speed}"
        elif key == 'KP_Left' or key == '4': cmd = f"MAN:LFT,{speed}"
        elif key == 'KP_Right' or key == '6': cmd = f"MAN:RGT,{speed}"
        elif key == 'space': 
            self.waypoints_list.configure(state="normal")
            self.waypoints_list.insert("end", f"Waypoint Saved\n")
            self.waypoints_list.configure(state="disabled")
            return
            
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

    def load_mission(self):
        try:
            filename = filedialog.askopenfilename(defaultextension=".txt", filetypes=[("Text Files", "*.txt"), ("All Files", "*.*")])
            if not filename: return
            
            with open(filename, "r", encoding="utf-8") as f:
                content = f.read()
                self.log_box.delete("1.0", "end")
                self.log_box.insert("end", content)
            
            # Optional: Display loaded filename in label if exists, or just log it
            print(f"Loaded mission from {filename}")
            
        except Exception as e:
            messagebox.showerror("Error", f"Could not load: {e}")

if __name__ == "__main__":
    app = DashboardApp()
    app.mainloop()
