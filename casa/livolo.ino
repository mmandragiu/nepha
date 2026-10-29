// livolo - intrerupatoarele Livolo din dormitor, prin emitatorul de 433 MHz
//
// Intrerupatorul Livolo asculta o telecomanda: fiecare buton trimite 23 de biti
// (16 = "numele" telecomenzii, 7 = butonul). Emitatorul nostru se preface ca e telecomanda.
// Protocolul l-am luat din biblioteca LivoloTx (github.com/bitlinker/LivoloTx, licenta MIT).
//
// Butonul obisnuit COMUTA (aprins <-> stins), iar intrerupatorul nu ne spune cum e acum.
// Ca sa fim siguri de rezultat:
//   stins   = butonul "stinge tot"
//   aprins  = "stinge tot", apoi butonul nostru (din stins devine aprins)
// Livolo e doar aprins / stins: de la LIVOLO_PRAG in sus e aprins. Mai jos (lumina de noapte,
// 20% cand te misti noaptea) ramane stins, ca sa nu aprinda lustra peste cei care dorm.
// Daca cineva apasa intrerupatorul de mana, casa nu afla; ne potrivim la urmatoarea schimbare.
//
// Legarea (o singura data, pentru fiecare intrerupator, la cap de scara pe amandoua):
//   tineti degetul pe intrerupator pana piuie (~5 s), apoi comanda LIVOLO LEAGA (sau
//   butonul din aplicatie) in urmatoarele secunde.

#define LIVOLO_STINGE_TOT 106

static uint8_t livoloAprins = 255;     // 255 = nu stim (la pornire si dupa legare)

static void livoloTrimite(uint8_t buton) {
  uint32_t cod = (buton & 0x7F) | ((uint32_t)LIVOLO_TELECOMANDA << 7);
  for (uint8_t r = 0; r < 150; r++) {
    // ~7 ms fara intreruperi: altfel radioul din casa (care aude tot ce trimitem) ne strica timpii
    noInterrupts();
    bool sus = false;
    digitalWrite(PIN_LIVOLO, HIGH);      // inceputul: un puls lung
    delayMicroseconds(525);
    digitalWrite(PIN_LIVOLO, LOW);
    for (int8_t b = 22; b >= 0; b--) {
      if (cod >> b & 1) {                // 1 = o schimbare dupa 315 us
        delayMicroseconds(315);
        sus = !sus;
        digitalWrite(PIN_LIVOLO, sus);
      } else {                           // 0 = doua schimbari scurte, de 120 us
        delayMicroseconds(120);
        digitalWrite(PIN_LIVOLO, !sus);
        delayMicroseconds(120);
        digitalWrite(PIN_LIVOLO, sus);
      }
    }
    interrupts();
  }
  digitalWrite(PIN_LIVOLO, LOW);
  usiDupaPauza();
}

void livoloLeaga() {
  livoloTrimite(LIVOLO_BUTON);
  livoloAprins = 255;
}

// o trimitere dureaza ~1 s, asa ca trimitem doar cand se schimba ceva
void livoloPas() {
  uint8_t vrem = casa.camere[CAMERA_LIVOLO].luminaTinta >= LIVOLO_PRAG;
  if (vrem == livoloAprins) return;
  livoloAprins = vrem;
  livoloTrimite(LIVOLO_STINGE_TOT);
  if (vrem) livoloTrimite(LIVOLO_BUTON);
}
