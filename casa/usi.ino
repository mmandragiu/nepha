// usi - senzorii wireless de pe usi si geamuri (433 MHz)
//
// Fiecare senzor are un emitator (telecomanda modificata, cu MC-38) care trimite
// un cod unic cat timp usa e deschisa. Receptorul SYN480R e pe pinul D2, iar
// biblioteca RCSwitch transforma semnalul in numarul trimis.
// Telecomanda se mai opreste singura dupa un timp, chiar daca usa e tot deschisa.
// Atunci usa apare inchisa; din panou (atingand starea) o putem pune cum e de fapt,
// iar starea pusa de mana ramane asa pana cand senzorul transmite din nou.
// Codurile nu le scriem in program: le "invatam" din panou (sau cu comanda
// INVATA 0 pe Serial) si le pastram in EEPROM.

RCSwitch radio;
uint32_t coduriRF[NR_SENZORI_RF];
uint32_t ultimulCod[NR_SENZORI_RF];    // millis() cand am auzit ultima data senzorul
int8_t slotDeInvatat = -1;
uint32_t invatPanaLa = 0;
uint8_t usiManual = 0;                 // bitul i = starea senzorului i a fost pusa din panou

void usiInit() {
  radio.enableReceive(digitalPinToInterrupt(PIN_RF));
  for (uint8_t i = 0; i < NR_SENZORI_RF; i++) coduriRF[i] = citesteCodRF(i);
}

bool senzorDeschis(uint8_t slot) {
  return casa.usiDeschise & (1 << slot);
}

// dinPanou = am pus-o noi din ecran, nu a simtit-o senzorul (atunci nu porneste alarma)
static void schimbaSenzor(uint8_t slot, bool deschis, bool dinPanou) {
  if (deschis) casa.usiDeschise |= 1 << slot;
  else casa.usiDeschise &= ~(1 << slot);
  // geamurile le stie si termostatul (pentru pauza la incalzire)
  int8_t cam = CAMERA_SENZOR[slot];
  if (TIP_SENZOR[slot] == FEREASTRA && cam >= 0 && !inScenariu()) casa.camere[cam].geamDeschis = deschis;
  trimiteEveniment(EV_USA, slot, deschis);
  if (deschis && !dinPanou) alarmaSenzor(slot);
}

void usiInvata(uint8_t slot) {
  if (slot >= NR_SENZORI_RF) return;
  slotDeInvatat = slot;
  invatPanaLa = millis() + 20000UL;
  Serial.print(F("Deschide usa / geamul cu senzorul "));
  Serial.println(slot);
}

// USA <slot> <0|1> - din panou: "usa asta e de fapt inchisa / deschisa"
void usiSeteaza(uint8_t slot, bool deschis) {
  if (slot >= NR_SENZORI_RF) return;
  usiManual |= 1 << slot;
  if (senzorDeschis(slot) != deschis) schimbaSenzor(slot, deschis, true);
}

void usiSterge(uint8_t slot) {
  if (slot >= NR_SENZORI_RF) return;
  coduriRF[slot] = 0;
  salveazaCodRF(slot, 0);
}

void usiVerifica() {
  if (radio.available()) {
    uint32_t cod = radio.getReceivedValue();
    uint8_t biti = radio.getReceivedBitlength();
    radio.resetAvailable();

    if (cod != 0 && biti >= 12) {        // codurile foarte scurte sunt de obicei zgomot
      if (slotDeInvatat >= 0) {
        bool folosit = false;
        for (uint8_t i = 0; i < NR_SENZORI_RF; i++) if (coduriRF[i] == cod && i != slotDeInvatat) folosit = true;
        if (!folosit) {
          coduriRF[slotDeInvatat] = cod;
          salveazaCodRF(slotDeInvatat, cod);
          trimiteEveniment(EV_SENZOR_INVATAT, slotDeInvatat, 1);
          Serial.print(F("Senzor invatat, cod "));
          Serial.println(cod);
          slotDeInvatat = -1;
          return;
        }
      }
      bool cunoscut = false;
      for (uint8_t i = 0; i < NR_SENZORI_RF; i++) {
        if (coduriRF[i] != 0 && coduriRF[i] == cod) {
          cunoscut = true;
          ultimulCod[i] = millis();
          usiManual &= ~(1 << i);      // senzorul transmite: el stie mai bine
          if (!senzorDeschis(i)) schimbaSenzor(i, true, false);
        }
      }
      if (!cunoscut) {
        Serial.print(F("Cod radio necunoscut: "));
        Serial.print(cod);
        Serial.print(F(" ("));
        Serial.print(biti);
        Serial.println(F(" biti)"));
      }
    }
  }

  // n-am mai auzit senzorul de cateva secunde -> usa s-a inchis
  for (uint8_t i = 0; i < NR_SENZORI_RF; i++) {
    if (senzorDeschis(i) && !(usiManual & (1 << i)) && millis() - ultimulCod[i] > TIMP_INCHIDERE_MS)
      schimbaSenzor(i, false, false);
  }

  if (slotDeInvatat >= 0 && (int32_t)(millis() - invatPanaLa) >= 0) {
    trimiteEveniment(EV_SENZOR_INVATAT, slotDeInvatat, 0);
    slotDeInvatat = -1;
  }
}
