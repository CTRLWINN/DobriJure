# Korisničke Upute: WSC 2026 Dashboard (Mission Control)

Dobrodošli u WSC 2026 Mission Control! Ove upute namijenjene su učenicima, kako biste što brže savladali upravljanje autonomnim robotom pomoću naše Dashboard aplikacije. 

Kao budući inženjeri, tehničari i programeri, važno je razumjeti ne samo **kako** kliknuti na određeni gumb, već i **zašto**. Aplikacija je osmišljena prema industrijskim standardima, kombinirajući izravno upravljanje parametrima i algoritamski pristup rješavanju problema.

Posebnu pažnju posvetit ćemo metodologiji **Teach-in** (metoda učenja robota korak po korak) te postupcima pohrane (**Eksport / Import**) vaših algoritamskih rješenja.

---

## 1. Koncept Aplikacije: 3 Glavne Kartice

Nakon uspješnog BLE (Bluetooth Low Energy) spajanja s robotom putem gumba **Connect BLE (HC-02)** u bočnoj traci (lijevo), aplikacija nudi tri glavna okruženja (kartice):

1. **Kalibracija kamere** - Područje u kojem podešavamo senzore i strojnu viziju (boje).
2. **Učenje (Manual)** - Ovdje algoritmiziramo ponašanje robota metodom *teach-in*. Slažemo naredbe u niz.
3. **Autonomno** - Ovdje importiramo pripremljeni kod i puštamo robota da zadatak odradi potpuno samostalno ("hands-off").

---

## 2. Učenje robota: Teach-in metodologija (Kartica "Učenje")

Teach-in podrazumijeva učenje robota pokazivanjem i testiranjem. Robota navodimo da napravi određeni korak, zabilježimo taj korak, te prelazimo na sljedeći.

Kartica **Učenje (Manual)** podijeljena je u 3 ključna stupca koji prate inženjerski tijek rada (Workflow):

### Stupac 1: BIBLIOTEKA (Dostupne Funkcije)
Sve naredbe svedene su na gradivne blokove (funkcije). 
* **Navigacijske naredbe:** `VOZI RAVNO`, `VOZI (L/D)`, `OKRENI (Spot)`, `SKRENI (Pivot)`.
* **Manipulacija:** `RUKA` (aktivacija robotske ruke/hvataljke).
* **Osjetila:** `PROVJERI SENZOR`, `KALIBRIRAJ IMU` (resetiranje žiroskopa za preciznost).
* **Kontrola toka (Logika):** `AKO JE TOČNO`, `INAČE`, `KRAJ BLOKA` (slično `if-else` strukturi u programiranju).

> 🎓 **Pedagoška napomena:** U programiranju robota, uvijek moramo razlikovati "Spot turn" (okret u mjestu oko svoje osi) i "Pivot turn" (skretanje u luku, npr. oko jednog kotača).

### Stupac 2: INSPECTOR (Podešavanje Parametara)
Kada u Biblioteci kliknete na željenu funkciju (npr. `VOZI RAVNO`), u Inspektoru morate upisati ulazne argumente (parametre).
* Primjer za vožnju: Upišite udaljenost u centimetrima (npr. `50`) i brzinu (npr. `150`).
* Na dnu inspektora možete testirati i ugađati direktne parametre za servo motore (Manipulator) te spremati pozicije (presete).

Kada ste unijeli vrijednosti, pritisnite gumb **"IZVRŠI I DODAJ"**.
**Što se time postiže?** Robot će odmah izvesti zadanu naredbu kako biste vidjeli je li rezultat dobar. Zatim se ta naredba trajno zapisuje u memoriju (u Editor).

### Stupac 3: EDITOR MISIJE (Izgradnja logike)
Svakim klikom na "Izvrši i dodaj", naredbe se slažu u "Editor Misije". Ovaj stupac je reprezentacija vašeg koda.
* **Pogriješili ste redoslijed?** Koristite plave strelice (▲, ▼) za zamjenu redoslijeda zadataka.
* **Trebate promijeniti vrijednost?** Kliknite na plavi gumb **"E" (Edit)**. Naredba se vraća u Inspector, robot **neće** ponovno izvesti kretnju, već ćete samo spasiti novu vrijednost pritiskom na "SPREMI PROMJENE".
* **Brisanje:** Crveni gumb **"X"** zauvijek briše taj korak iz rutine.

---

## 3. EKSPORT: Spašavanje misije (Zapisivanje u fajl)

Nakon što ste pomoću Teach-in metode kreirali sekvencu (rutinu) s kojom ste zadovoljni, potrebno ju je sačuvati. Zamislite to kao "Save As" vašeg izvornog koda.

1. U trećem stupcu (*Editor Misije*) nalaze se dva gumba: "Eksportiraj" i "Očisti sve".
2. Kliknite na zeleni gumb **EKSPORTIRAJ (misija.txt)**.
3. Otvorit će vam se standardni Windows prozor za spremanje datoteke.
4. Upišite smisleno ime datoteke, na primjer `Misija_Krug_Ponedjeljak.txt` ili koristite *defaultno* `misija.txt`.
5. Kliknite "Save" (Spremi).

Sustav će u pozadini prevesti sve vaše grafičke korake u sirovi, standardizirani `JSON` (JavaScript Object Notation) format koji je vrlo čest u modernim tehnologijama komunikacije sustava. Svaki redak u .txt datoteci predstavljat će jedan precizan `JSON` objekt.

> 🛠️ **Primjer iz prakse:** Ako trebate raditi modifikacije kod kuće bez robota, ovu izvezenu \`.txt\` datoteku možete otvoriti u Notepad-u i mijenjati zadane vrijednosti!

---

## 4. IMPORT: Autonomno izvođenje (Kartica "Autonomno")

Dolazi dan natjecanja ili završni ispit. Više nema spajanja kablovima, manualnog pogađanja udaljenosti niti ugađanja. Prelazite na treću karticu: **Autonomno**.

### a) Učitavanje pripremljene sekvence
1. S desne strane ekrana locirajte sekciju "Učitana Misija".
2. Kliknite na gumb **Učitaj (misija.txt)**.
3. Locirajte `.txt` datoteku koju ste jučer/ranije eksportirali na vašem računalu i potvrdite.
4. Vaš će se kod u izvornom obliku učitati u veliki prozor na ekranu aplikacije.

### b) Verifikacija i Pokretanje
1. Provjerite u velikom prozoru nalaze li se naredbe koje očekujete.
2. Provjerite telemetriju s lijeve strane (očitanje senzora, udaljenosti, prepreka) da biste bili sigurni da se robot starta iz korektne pozicije.
3. Kliknite veliki, zeleni gumb **POKRENI MISIJU**.

**Što se događa ispod haube?**
Dashboard prelazi u asinkroni mod ("Thread"). Čita jednu po jednu liniju iz tekstualnog polja i samostalno ju šalje robotu putem Bluetootha. Robot izvede pomak baze ruke ili okret za 90°, javi nazad kompjuteru "Spreman", a aplikacija prelazi na sljedeći korak.

> ⚠️ **Sigurnost na prvom mjestu (E-Stop):**
> Ako primijetite da će robot udariti u prepreku ili pasti sa stola, odmah kliknite crveni gumb **STOP** pored gumba za pokretanje. Sustav momentalno šalje prekidni kod robotu i pauzira daljnje slanje rutine!

---

## Zaključak

Moderna industrija zahtijeva repetitivnost. Razdvajanje softverskog sučelja na **fazu učenja (Teach-in i Eksport)** i **fazu izvršavanja (Import i Auto mode)** odgovara stvarnom principu rada industrijskih robota (Siemens, KUKA, ABB). Vježbom putem ovog Dashboarda u biti simulirate ulogu modernog inženjera robotike u pravoj tvornici! Sretno programiranje!
