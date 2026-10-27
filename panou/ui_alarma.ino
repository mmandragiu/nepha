// ui_alarma - armare / dezarmare, senzorii de pe usi si geamuri, tastatura pentru PIN.
// PIN-ul nu e tinut minte pe panou: il trimitem lui Arduino si el decide.

int8_t slotInvatat = -1;
uint32_t invatPornitLa = 0;
String pinScris;
uint32_t pinDeschisLa = 0;
String mesajPin;

static const char *explicatieAlarma(uint8_t s) {
  switch (s) {
    case 1: return "Aveți 30 de secunde să ieșiți.";
    case 2: return "Doar ușile și geamurile.";
    case 3: return "Casa economisește cât e goală.";
    case 4: return "Introduceți PIN-ul.";
    case 5: return "Introduceți PIN-ul ca s-o opriți.";
  }
  return "";
}

static void cardStareAlarma() {
  if (!schimbat(0, String(casa.alarma))) return;
  for (uint8_t id = 60; id <= 62; id++) stergeZona(id);
  bool pericol = casa.alarma >= 4;
  uint16_t fundal = pericol ? C_ROSU_D : C_CARD;
  uint16_t c = pericol ? C_ROSU : (casa.alarma == 0 ? C_TEXT2 : C_VERDE);
  card(12, 50, 220, 216, fundal);
  iconScut(40, 82, c, fundal);
  scrie(textAlarma(casa.alarma), 60, 82, F_TEXT, pericol ? C_ROSU : C_TEXT, fundal, textdatum_t::middle_left);
  paragraf(explicatieAlarma(casa.alarma), 28, 106, 190, F_MIC, C_TEXT, fundal, casa.alarma == 0 ? 3 : 5);
  if (casa.alarma == 0) {
    buton("Plec de acasă", 24, 176, 196, 38, 60, true);
    buton("Noapte", 24, 220, 196, 38, 61);
  } else {
    buton("Dezarmează", 24, 220, 196, 38, 62, true);
  }
}

static void cardSenzori() {
  bool ascult = slotInvatat >= 0 && millis() - invatPornitLa < 20000;
  if (!ascult) slotInvatat = -1;
  if (!schimbat(1, String(casa.usi) + slotInvatat)) return;
  card(240, 50, 228, 216);
  scrie("Uși și geamuri", 256, 62, F_MIC, C_TEXT2, C_CARD);
  for (uint8_t i = 0; i < NR_SENZORI_RF; i++) {
    int y = 84 + i * 44;
    if (i) tft.drawFastHLine(256, y - 4, 196, C_GRI_D);
    scrie(NUME_SENZORI[i], 256, y + 18, F_MIC, C_TEXT, C_CARD, textdatum_t::middle_left);
    if (slotInvatat == i) pastila("ascult...", 452, y + 8, C_SOARE, C_SOARE_D, textdatum_t::top_right);
    else if (casa.usi & (1 << i)) pastila("deschis", 452, y + 8, C_CALD, C_CALD_D, textdatum_t::top_right);
    else pastila("închis", 452, y + 8, C_TEXT2, C_GRI_D, textdatum_t::top_right);
    adaugaZona(240, y, 128, 40, 70 + i);    // numele: invatam senzorul
    adaugaZona(368, y, 100, 40, 74 + i);    // starea: o schimbam de mana
  }
}

void desenAlarma(bool tot) {
  cardStareAlarma();
  cardSenzori();
}

void atingeAlarma(uint8_t id) {
  if (id == 60) trimiteArduino("ALARMA PLECAT");
  if (id == 61) trimiteArduino("ALARMA ACASA");
  if (id == 62) deschidePin();
  if (id >= 70 && id < 70 + NR_SENZORI_RF) {
    // invatam un senzor nou: dupa apasare, deschideti usa / geamul in 20 de secunde
    slotInvatat = id - 70;
    invatPornitLa = millis();
    char cmd[12];
    snprintf(cmd, sizeof(cmd), "INVATA %d", slotInvatat);
    trimiteArduino(cmd);
    dateNoiUI = true;
  }
  if (id >= 74 && id < 74 + NR_SENZORI_RF) {
    // telecomanda de pe usa se mai opreste singura: aici ii spunem casei cum e de fapt
    uint8_t slot = id - 74;
    bool deschis = !(casa.usi & (1 << slot));
    char cmd[12];
    snprintf(cmd, sizeof(cmd), "USA %d %d", slot, deschis);
    trimiteArduino(cmd);
    casa.usi ^= 1 << slot;
    dateNoiUI = true;
  }
}

// ---------------------------------------------------------------- tastatura PIN

void deschidePin() {
  pinScris = "";
  mesajPin = "";
  pinDeschisLa = millis();
  schimbaEcran(E_PIN);
}

static const char *TASTE[12] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "<", "0", "OK"};

void desenPin(bool tot) {
  if (tot) {
    for (uint8_t k = 0; k < 12; k++) {
      int x = 244 + (k % 3) * 76, y = 50 + (k / 3) * 54;
      buton(TASTE[k], x, y, 70, 48, 80 + k, k == 11);
    }
    buton("Înapoi", 24, 218, 196, 40, 79);
  }
  uint32_t secunde = (millis() - pinDeschisLa) / 1000;
  String semn = pinScris + "|" + mesajPin + "|" + casa.alarma + (casa.alarma == 4 ? String(secunde) : "");
  if (!schimbat(0, semn)) return;
  bool pericol = casa.alarma >= 4;
  uint16_t fundal = pericol ? C_ROSU_D : C_CARD;
  card(12, 50, 220, 160, fundal);
  scrie("PIN-ul alarmei", 28, 64, F_TEXT, C_TEXT, fundal);
  for (uint8_t i = 0; i < 4; i++) {
    int cx = 52 + i * 40;
    if (i < pinScris.length()) tft.fillSmoothCircle(cx, 112, 9, C_TEXT);
    else {
      tft.fillSmoothCircle(cx, 112, 9, C_MARGINE);
      tft.fillSmoothCircle(cx, 112, 7, fundal);
    }
  }
  String jos = mesajPin;
  if (!jos.length() && casa.alarma == 4) jos = "Mai aveți " + String(secunde < 20 ? 20 - secunde : 0) + " secunde";
  if (jos.length()) paragraf(jos, 28, 142, 190, F_MIC, pericol ? C_ROSU : C_TEXT2, fundal, 3);
  if (casa.alarma == 4) dateNoiUI = true;       // ca sa curga secundele
}

void atingePin(uint8_t id) {
  if (id == 79) {
    schimbaEcran(E_ALARMA);
    return;
  }
  uint8_t k = id - 80;
  if (k < 9 || k == 10) {
    if (pinScris.length() < 4) pinScris += TASTE[k];
    mesajPin = "";
  } else if (k == 9) {
    if (pinScris.length()) pinScris.remove(pinScris.length() - 1);
  } else if (k == 11 && pinScris.length()) {
    trimiteArduino(("ALARMA OPRIT " + pinScris).c_str());
    pinScris = "";
    mesajPin = "Verific...";
  }
  dateNoiUI = true;
}

// raspunsul lui Arduino la PIN (evenimentele 13 si 16) - apelat din ui.ino
void pinRaspuns(const EvenimentP &e) {
  if (e.cod == 16) mesajPin = textEveniment(e);
  dateNoiUI = true;
}
