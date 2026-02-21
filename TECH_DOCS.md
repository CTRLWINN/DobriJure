# 📘 WSC 2026 Autonomni Robot - Tehnička Dokumentacija Sustava

## 1. Sažetak Arhitekture i Namjena Repozitorija
Ovaj repozitorij obuhvaća cjelokupni upravljački softver, firmver i prateće alate za napredni, modularni autonomni robot dizajniran za natjecanje **WorldSkills Croatia 2026** u disciplini Robotika. Repozitorij je dizajniran kao cjelovito *"Full-Stack"* robotsko rješenje koje usko integrira mehatroniku niske razine (mikrokontroleri i senzori), lokalno ugrađeno procesiranje umjetne inteligencije (*Edge AI*) i visokorazinske nadzorne aplikacije na računalu (*GUI Dashboard*).

Primarna namjena ovog repozitorija je formiranje jedinstvenog prostora za upravljanje verzijama koda, omogućavanje robusne baze s jasnom separacijom modula te standardiziranje protokola komunikacije između podsustava, čime se timu inženjera maksimalno ubrzava testiranje operacija i razvoj složenih sekvenci poput kinematike i navigacije.

## 2. Sistemska Arhitektura (Softver)
Funkcionalnost robota raslojena je u četiri nezavisne razine:

### 2.1 Podsustav Kontrolera Upravljanja i Prikupa Podataka (`Robot_Main`)
- **Upravljačka Jezgra:** Centralni nadzor provodi Arduino Mega 2560 koja koordinira rad mehatroničkih cjelina na principu determinističke petlje izvršavanja (*State Machine*).
- **Zatvorena Kontrola Translacije (`Kretanje.h`):** Pogon platforme je diferencijalni (upravljan L298N mosnim h-driverom), u potpunosti upravljan optimiziranim PID algoritmom koji osigurava glatko praćenje crne linije. 
- **Senzorska Fuzija (Sensor Fusion):** Lokacija u prostoru izravno se derivira kombinacijom inkrementalnih kvadraturnih enkodera upravljanih prekidnim vektorima (*Interrupts*) te 9-osnog IMU sustava (*LSM9DS1* preko I2C). 
  - IMU ugrađeni žiroskop integriran je u kompenzacijsku petlju za održavanje visoko-preciznog apsolutnog pravca (*Heading*), kompenzirajući trenje i asimetriju motora.
  - Akcelerometar pruža sigurnosnu značajku detekcije "Jerka" za nagli prekid operacija unutar *Grand Slam* misije i prevenciju zapinjanja uz prepreke.
  - Senzorske grupe za nadzor rubnika staze (*Lane Assist*) rade u "pasivnom override" modu ne bi li robotu prioritetno nametnuli zadržavanje unutar limita, sprječavajući fatalne greške pri gubitku trake sa glavnog analognog 5-kanalnog IR senzoričkog panela.

### 2.2 Podsustav Kinematike Manipulatora (`Manipulator.h`)
- Zglobna interakcija postiže se kroz kompleksni 6-DOF manipulator pogonjen serijom od sedam namjenskih DC serva povezanih putem perifernog I2C koprocesora za generiranje PWM signala (*PCA9685*). 
- **Pametna Hvataljka i Eye-in-Hand Pristup:** Ovaj mehanizam, pored mehaničkih ruku, montiranu na glavi sadrži i *Nicla Vision* kameru, te integrira induktivne kapacitivne senzore i Time-of-Flight lidare radi precizne autovalidacije prilikom sakupljanja elemenata (Boca, Limenka, Spužva).
- Sposobnost izvođenja *Soft-Start* interpoliranog kretanja radikalno smanjuje "inrush" povlačenje električne energije na servu prilikom naglih zaustavljanja i garantira težišni ekvilibrij mehaničkog sustava te odlaganja predmeta u privremene "Krovne" spremnike (Slot 1 i Slot 2).

### 2.3 Konvolucijske Neuronske Mreže (Edge AI `NiclaVision`)
- Kao hardver usko vezan za domenu računalnog vida (*Computer Vision*), Nicla Vision modul koristi vlastite procesore i memorijsku matricu te na glavnu `Serial3` UART liniju komunicira samo strukturirane i klasificirane rezulate (primjerice "BOCA, kut 45, udaljenost 15cm") detekcije boja i uzoraka (Neuronske mreže). Omogućuje *Smart Start* – detekciju kalibracijskih QR kodova te autonomno traženje meta.

### 2.4 Mission Control Ekosustav Naredbi i Telemetrije (`dashboard_v2.py`)
- Python 3 *CustomTkinter* aplikacija predstavlja "Cockpit" robota. Realizacija serijske komunikacije osniva se na brzim, asinkronim (BLE bežičnim posredstvom HC-02) JSON okvirnicama.
- Sustav ne šalje podatke nasumično, nego je stvoren protokol gdje se koristi pristup *Command-Response*. Ovaj način osigurava da GUI aplikacija, dok stvara `misija.txt` u svojem *Teach-In* modalitetu i kasnije to pošalje Arduino kontroleru, čeka da kontroler asertivno potvrdi (*Acknowledge*) dosezanje ciljne udaljenosti prije iniciranja narednog pomaka (npr. čekanje validacije sa ugrađenih motornih enkodera da bi se započeo postupak *ARM:Uzmi_Boca*).

## 3. Tehničke Smjernice Organizacije i Dizajna (Senior Engineer Perspektiva)
Kako bi modularnost nastavila funkcionirati kroz daljnje operativne nadogradnje utemeljene na ovom repozitoriju:
1. **Asinkronost u Jezgri:** Strogo se referira na "Non-blocking" arhitekturu `Robot_Main.ino`. Blokirajuće funkcije (`delay()` funkcije i teške *while* petlje) uništavaju reaktivni I2C IMU FIFO buffer prozor i UART telemetrijsku sinkronizaciju te time ugrožavaju integritet operativne matrice zatvorene petlje.
2. **Razdvojeno Napajanje (Power Distribution):** Potreba za potpunom galvanskom i stabilizacijskom izolacijom logičkih cjelina (Nicla, Arduino, Senzori 3.3/5V) i snagaškog sklopa (L298N drivera te 6-DOF ruku) spojenih izravno na Lipo/regulirane visoko-strujne bukove izvode, sprječava nestabilnost (*brown-outs*) u trenucima vršnih pogonskih momenta ubrzanja.
3. **Konstantna Skalarnost Kontrole:** Centralizacija hardverskih i memorijskih konstanti prebačena je u jedinstveni `HardwareMap.h` - modifikacije pinova rješavaju isključivo i striktno tamo na hardverskoj abastrakciji bez interferencije sa *Core* upravljačkom logikom, podržavajući visoke stupnjeve sigurnog inženjerskog skaliranja.

Sinergija C++ ugrađene logike i optimizacija Pythona, okružena jasnim ugovorima o podacima putem JSON formata omogućit će jednostavan "Deployment" te zavidnu brzinu ciklusa implementacije i testiranja u rigoroznim zahtjevima "Grand Slam" natjecanja.
