// ui_setari - reteaua, legarea unui telefon, modul casei (smart / clasic), temperatura de confort,
// calibrarea ecranului

// temperatura de confort o tine Arduino (o trimite in stare, tc=); pana o primim, 21.5
static float confort() {
  return casa.confort > 0 ? casa.confort : 21.5;
}

void desenSetari(bool tot) {
  uint32_t cod = codDeAfisat();
  String semn = numeRetea() + adresaIP() + casa.mod + zecimal(confort(), 1) + mqttConectat() + cod +
                (cod ? secundeImperechere() / 10 : 0);
  if (!schimbat(0, semn)) return;
  for (uint8_t id = 93; id <= 99; id++) stergeZona(id);
  card(12, 50, 456, 216);

  // reteaua
  scrie("Rețea", 28, 62, F_MIC, C_TEXT2, C_CARD);
  String linie1, linie2;
  if (cod) {
    // telefonul scrie codul asta in aplicatie (Securitate & Setari -> Leaga telefonul)
    char text[48];
    snprintf(text, sizeof(text), "Cod pentru telefon: %03u %03u", (unsigned)(cod / 1000), (unsigned)(cod % 1000));
    linie1 = text;
    linie2 = "Scrieți-l în aplicație (mai e valabil " + String(secundeImperechere()) + " s)";
  } else if (online()) {
    linie1 = numeRetea() + " · " + adresaIP();
    linie2 = String("Aplicația: adresa ") + adresaIP() + " · prin internet: " + (mqttConectat() ? "da" : "nu") +
             " · " + idServer;
  } else if (modAP()) {
    linie1 = "Rețeaua panoului: " AP_NUME " (parola " AP_PAROLA ")";
    linie2 = "Pe telefon: conectați-vă la ea și deschideți 192.168.4.1/wifi";
  } else {
    linie1 = "Mă conectez...";
  }
  scrie(linie1, 28, 82, F_TEXT, C_TEXT, C_CARD);
  if (linie2.length()) scrie(linie2, 28, 108, F_MIC, C_TEXT2, C_CARD);

  tft.drawFastHLine(28, 132, 424, C_GRI_D);
  scrie("Cum se poartă casa", 28, 150, F_MIC, C_TEXT2, C_CARD, textdatum_t::middle_left);
  buton("Smart", 250, 136, 98, 32, 95, casa.mod == 1);
  buton("Clasic", 354, 136, 98, 32, 96, casa.mod == 0);

  scrie("Temperatura de confort", 28, 196, F_MIC, C_TEXT2, C_CARD, textdatum_t::middle_left);
  buton("−", 250, 180, 44, 32, 97);
  scrie(grade(confort()), 324, 197, F_TEXT, C_TEXT, C_CARD, textdatum_t::middle_center);
  buton("+", 354, 180, 44, 32, 98);

  buton("Jurnal", 28, 224, 108, 34, 94);
  buton("Leagă telefonul", 144, 224, 160, 34, 93, cod != 0);
  buton("Calibrează", 312, 224, 140, 34, 99);
}

void atingeSetari(uint8_t id) {
  if (id == 95) trimiteArduino("MOD SMART");
  if (id == 96) trimiteArduino("MOD CLASIC");
  if (id == 97 || id == 98) {
    casa.confort = constrain(confort() + (id == 98 ? 0.5 : -0.5), 16.0, 26.0);   // ca in aplicatie
    char cmd[24];
    snprintf(cmd, sizeof(cmd), "TEMP CONFORT %d", (int)lroundf(casa.confort * 10));
    trimiteArduino(cmd);
    dateNoiUI = true;
  }
  if (id == 93) {
    incepeImperechere();
    dateNoiUI = true;
  }
  if (id == 94) schimbaEcran(E_JURNAL);
  if (id == 99) calibreazaAtingerea();
}
