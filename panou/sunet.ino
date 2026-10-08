// sunet - difuzorul si LED-ul RGB de pe placa
//
// Difuzorul: un "clic" cand atingem ecranul, bipuri cand se armeaza alarma,
// sirena cand suna. LED-ul: verde din cand in cand = alarma armata, rosu = alarma.

uint8_t sunetStare = 0xFF;    // ce sunet facem acum (dupa starea alarmei)
uint32_t sunetDin = 0;

void bip(uint16_t frecventa, uint16_t durata) {
  tone(PIN_DIFUZOR, frecventa, durata);
}

void sunetPas() {
  uint8_t s = casa.alarma;
  uint32_t m = millis();
  if (s != sunetStare) {
    sunetStare = s;
    sunetDin = m;
    noTone(PIN_DIFUZOR);
  }
  uint32_t t = m - sunetDin;
  static uint32_t ultimBip = 0;
  if (s == 5) {                                  // sirena: doua tonuri care se schimba
    static bool sus = false;
    if (m - ultimBip > 350) {
      ultimBip = m;
      sus = !sus;
      tone(PIN_DIFUZOR, sus ? 1400 : 900);
    }
  } else if (s == 4 && m - ultimBip > 1000) {    // intrare: bip in fiecare secunda
    ultimBip = m;
    bip(2200, 80);
  } else if (s == 1 && m - ultimBip > 2000 && t < 31000) {   // iesire: bip la 2 secunde
    ultimBip = m;
    bip(1800, 40);
  }
}

static void led(bool r, bool g, bool b) {
  digitalWrite(PIN_LED_R, r ? LOW : HIGH);    // LED-ul se aprinde pe LOW
  digitalWrite(PIN_LED_G, g ? LOW : HIGH);
  digitalWrite(PIN_LED_B, b ? LOW : HIGH);
}

void ledPas() {
  uint32_t m = millis();
  switch (casa.alarma) {
    case 5: led((m / 150) % 2, false, false); break;            // rosu, repede
    case 4: led((m / 500) % 2, false, false); break;            // rosu, rar
    case 1: led(false, (m / 500) % 2, false); break;            // verde clipind: se armeaza
    case 2:
    case 3: led(false, m % 3000 < 60, false); break;            // o sclipire verde la 3 secunde
    default: led(false, false, false);
  }
}
