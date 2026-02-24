import json
import os
import sys
import shutil

def generiraj_cpp_kod(misija_fajl, izlazni_dir):
    """
    Prevodi JSON misiju u kompletan Arduino projekt (RobotStaza)
    s neblokirajućim State Machineom za izvođenje iste.
    To je najbitnije rješenje za postizanje potpune autonomije bez zastoja 
    u glavnoj Arduino petlji.
    """
    if not os.path.exists(misija_fajl):
        print(f"Greška: Datoteka {misija_fajl} ne postoji.")
        return

    try:
        with open(misija_fajl, 'r', encoding='utf-8-sig') as f:
            koraci = [json.loads(line) for line in f if line.strip()]
    except Exception as e:
        print(f"Greška pri čitanju JSON-a: {e}")
        return

    # 1. Kreiraj izlaznu mapu
    os.makedirs(izlazni_dir, exist_ok=True)
    
    # 2. Kopiraj hardverske datoteke iz Robot_Main
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    robot_main_dir = os.path.join(project_root, "Robot_Main")
    
    if os.path.exists(robot_main_dir):
        for filename in os.listdir(robot_main_dir):
            if filename.endswith(".cpp") or filename.endswith(".h"):
                src = os.path.join(robot_main_dir, filename)
                dst = os.path.join(izlazni_dir, filename)
                shutil.copy2(src, dst)
    else:
        print(f"Upozorenje: Izvorna mapa s C++ bibliotekama {robot_main_dir} ne postoji!")

    # 3. Izračun state machine skokova za if-else
    skokovi_if_true = {}
    skokovi_if_false = {}
    stack_if = []
    
    for i, step in enumerate(koraci):
        cmd = step.get("cmd")
        if cmd == "if_true":
            stack_if.append({'if_true': i, 'if_false': -1, 'end_if': -1})
        elif cmd == "if_false":
            if stack_if:
                stack_if[-1]['if_false'] = i
        elif cmd == "end_if":
            if stack_if:
                blok = stack_if.pop()
                if_false_idx = blok['if_false']
                if if_false_idx != -1:
                    # Ako if_true blok padne (uvjet=false), skoči U if_false granu
                    skokovi_if_true[blok['if_true']] = if_false_idx + 1
                    # Ako if_false dođe na red (znači prošli smo if_true uspješno), skoči izvan bloka
                    skokovi_if_false[if_false_idx] = i + 1
                else:
                    # Ako nema else grane, iz if skoči na kraj ako padne (uvjet=false)
                    skokovi_if_true[blok['if_true']] = i + 1

    # Handlanje nezatvorenih IF blokova (ako fali end_if na kraju fajla)
    while stack_if:
        blok = stack_if.pop()
        kraj_idx = len(koraci)
        if_false_idx = blok['if_false']
        if if_false_idx != -1:
            skokovi_if_true[blok['if_true']] = if_false_idx + 1
            skokovi_if_false[if_false_idx] = kraj_idx
        else:
            skokovi_if_true[blok['if_true']] = kraj_idx

    # 4. Generiraj Root Arduino fajl (RobotStaza.ino)
    izlazni_fajl = os.path.join(izlazni_dir, "RobotStaza.ino")
    
    with open(izlazni_fajl, 'w', encoding='utf-8') as f:
        f.write("// Generirano automatski preko State Machinea - WSC 2026 Team Ctrl+Win\n")
        f.write("#include <Arduino.h>\n")
        f.write("#include \"HardwareMap.h\"\n")
        f.write("#include \"Kretanje.h\"\n")
        f.write("#include \"Motori.h\"\n")
        f.write("#include \"Enkoderi.h\"\n")
        f.write("#include \"IMU.h\"\n")
        f.write("#include \"Manipulator.h\"\n\n")
        
        f.write("Manipulator ruka;\n")
        f.write("int trenutnoStanje = 0;\n")
        f.write("bool ocekujemGumb = true;\n")
        f.write("unsigned long cekajDo = 0;\n")
        f.write("bool cekamVrijeme = false;\n")
        f.write("bool uvjetTest = false;\n\n")
        
        f.write("void setup() {\n")
        f.write("    inicijalizirajHardware();\n")
        f.write("    inicijalizirajEnkodere();\n")
        f.write("    inicijalizirajIMU();\n")
        f.write("    stani();\n")
        f.write("    kalibrirajGyro();\n")
        f.write("    resetirajGyro();\n")
        f.write("    resetirajMag();\n")
        f.write("    ruka.parkiraj();\n")
        f.write("    \n")
        f.write("    // Inicijalizacija gumba za Autonomiju\n")
        f.write("    pinMode(2, INPUT_PULLUP);\n")
        f.write("    Serial.println(\"Započinjem testnu sekvencu misije... (Pritisnite gumb D2)\");\n")
        f.write("}\n\n")
        
        f.write("void loop() {\n")
        f.write("    ruka.azuriraj();\n")
        f.write("    azurirajIMU();\n\n")
        
        f.write("    // Čekanje početnog triger signala (START)\n")
        f.write("    if (ocekujemGumb) {\n")
        f.write("        if (digitalRead(2) == LOW) {\n")
        f.write("            delay(50); // Hardverski debounce\n")
        f.write("            if (digitalRead(2) == LOW) {\n")
        f.write("                ocekujemGumb = false;\n")
        f.write("                Serial.println(\"Gumb pritisnut! Počinjem s misijom...\");\n")
        f.write("            }\n")
        f.write("        }\n")
        f.write("        return;\n")
        f.write("    }\n\n")
        
        f.write("    // Non-blocking wait logike\n")
        f.write("    if (cekamVrijeme) {\n")
        f.write("        if (millis() < cekajDo) return;\n")
        f.write("        cekamVrijeme = false;\n")
        f.write("    }\n\n")
        
        f.write("    // Glavni State Machine\n")
        f.write("    switch (trenutnoStanje) {\n")
        
        for i, step in enumerate(koraci):
            cmd = step.get("cmd")
            f.write(f"        case {i}:\n")
            f.write(f"            Serial.print(\"Stanje \"); Serial.print({i}); Serial.println(\": {cmd}\");\n")
            
            if cmd == "straight":
                val = step.get("val", 0)
                spd = step.get("spd", 0)
                f.write(f"            voziRavno({val}, {spd});\n")
                # Krucijalno je simulirati delay bez blokiranja mjenjanjem varijable cekamVrijeme
                f.write(f"            cekajDo = millis() + 300;\n") 
                f.write(f"            cekamVrijeme = true;\n")
                f.write(f"            trenutnoStanje = {i+1};\n")
                
            elif cmd == "turn":
                val = step.get("val", 0)
                spd = step.get("spd", 0)
                f.write(f"            okreni({val}, {spd});\n")
                f.write(f"            cekajDo = millis() + 300;\n")
                f.write(f"            cekamVrijeme = true;\n")
                f.write(f"            trenutnoStanje = {i+1};\n")
                
            elif cmd == "pivot":
                val = step.get("val", 0)
                spd = step.get("spd", 0)
                f.write(f"            skreni({val}, {spd});\n")
                f.write(f"            cekajDo = millis() + 300;\n")
                f.write(f"            cekamVrijeme = true;\n")
                f.write(f"            trenutnoStanje = {i+1};\n")
                
            elif cmd == "arm":
                val = step.get("val", "PARKING")
                # Manipulator očekuje sekvencu preseta
                f.write(f"            ruka.zapocniSekvencu(\"{val}\");\n")
                f.write(f"            cekajDo = millis() + 1000;\n")
                f.write(f"            cekamVrijeme = true;\n")
                f.write(f"            trenutnoStanje = {i+1};\n")
                
            elif cmd == "check_sensor":
                sensor_type = step.get("type", "induktivni")
                if sensor_type == "induktivni":
                    f.write(f"            uvjetTest = ocitajInduktivni();\n")
                f.write(f"            trenutnoStanje = {i+1};\n")
                
            elif cmd == "cal_imu":
                f.write(f"            Serial.println(\"Kalibriram IMU...\");\n")
                f.write(f"            kalibrirajGyro();\n")
                f.write(f"            resetirajGyro();\n")
                f.write(f"            resetirajMag();\n")
                f.write(f"            cekajDo = millis() + 100;\n")
                f.write(f"            cekamVrijeme = true;\n")
                f.write(f"            trenutnoStanje = {i+1};\n")
                
            elif cmd == "if_true":
                target_ako_padne = skokovi_if_true.get(i, i+1)
                f.write(f"            if (!uvjetTest) {{\n")
                f.write(f"                trenutnoStanje = {target_ako_padne};\n")
                f.write(f"            }} else {{\n")
                f.write(f"                trenutnoStanje = {i+1};\n")
                f.write(f"            }}\n")
                
            elif cmd == "if_false":
                target_preskoci_else = skokovi_if_false.get(i, i+1)
                f.write(f"            if (uvjetTest) {{\n")
                f.write(f"                trenutnoStanje = {target_preskoci_else};\n")
                f.write(f"            }} else {{\n")
                f.write(f"                trenutnoStanje = {i+1};\n")
                f.write(f"            }}\n")
                
            elif cmd == "end_if":
                f.write(f"            trenutnoStanje = {i+1};\n")
                
            elif cmd == "wait":
                val = step.get("val", 1000)
                f.write(f"            cekajDo = millis() + {val};\n")
                f.write(f"            cekamVrijeme = true;\n")
                f.write(f"            trenutnoStanje = {i+1};\n")
            
            else:
                f.write(f"            trenutnoStanje = {i+1};\n")
            
            f.write("            break;\n\n")
            
        f.write(f"        case {len(koraci)}:\n")
        f.write("            Serial.println(\"=== Misija je uspješno završena! ===\");\n")
        f.write("            stani();\n")
        f.write("            trenutnoStanje++; // Zaustavlja izvršavanje switcha zauvijek u sljedećim iteracijama\n")
        f.write("            break;\n")
        
        f.write("        default:\n")
        f.write("            // Sigurnosna zona nakon kraja\n")
        f.write("            break;\n")
        f.write("    }\n")
        f.write("}\n")

    print(f"Uspjeh! RobotStaza projekt sa State Machine logikom spreman je u: {izlazni_dir}")

if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    
    input_file = os.path.join(script_dir, "misija.txt")
    output_dir = os.path.join(project_root, "RobotStaza")
    
    if len(sys.argv) > 1:
        input_file = os.path.abspath(sys.argv[1])
    if len(sys.argv) > 2:
        output_dir = os.path.abspath(sys.argv[2])
        
    generiraj_cpp_kod(input_file, output_dir)
