/*
 * SerialTest.ino
 * 
 * Izolirani Arduino C++ kod za testiranje DVOSMJERNE komunikacije 
 * između Nicle (Serial3) i Arduino ploče (Serial).
 */

void setup() {
  Serial.begin(115200);   
  Serial3.begin(9600); 
  
  while (!Serial) {
    ; // Čekaj da se otvori serijski monitor na računalu
  }
  
  Serial.println("=====================================");
  Serial.println("  Nicla <-> Arduino Mega (9600 bps)  ");
  Serial.println("  Dvosmjerna komunikacija spremna!   ");
  Serial.println("=====================================");
}

void loop() {
  // 1. PRIMANJE S NICLE: Osluškuj podatke koji dolaze na RX3 (Pin 15) i šalji na PC
  if (Serial3.available()) {
    char c = Serial3.read();
    Serial.print(c);
  }

  // 2. SLANJE PREMA NICLI: Uzmi ono što si tipkao u Serial Monitor i šalji na Nicla TX3 (Pin 14)
  if (Serial.available()) {
    char out = Serial.read();
    Serial3.print(out);
    
    // Opcionalno: eho ispis na ekranu, da znaš što si poslao
    Serial.print("Poslao na Niclu: ");
    Serial.println(out);
  }
}
