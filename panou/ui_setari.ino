// ui_setari - reteaua, modul casei (smart / clasic), temperatura de confort, calibrarea ecranului

float confortSetat = 21.5;     // panoul nu primeste setarea de la Arduino, o tinem minte aici

void desenSetari(bool tot) {
  String semn = numeRetea() + adresaIP() + casa.mod + zecimal(confortSetat, 1);
  if (!schimbat(0, semn)) return;
  for (uint8_t id = 94; id <= 99; id++) stergeZona(id);
  card(12, 50, 456, 216);

  // reteaua
  scrie("Rețea", 28, 62, F_MIC, C_TEXT2, C_CARD);
  String linie1, linie2;
  if (online()) {
    linie1 = numeRetea() + " · " + adresaIP();
    linie2 = "Pagina casei: http://nepha.local sau http://" + adresaIP();
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
  scrie(grade(confortSetat), 324, 197, F_TEXT, C_TEXT, C_CARD, textdatum_t::middle_center);
  buton("+", 354, 180, 44, 32, 98);

  buton("Jurnal", 28, 224, 140, 34, 94);
  buton("Calibrează ecranul", 250, 224, 202, 34, 99);
}

void atingeSetari(uint8_t id) {
  if (id == 95) trimiteArduino("MOD SMART");
  if (id == 96) trimiteArduino("MOD CLASIC");
  if (id == 97 || id == 98) {
    confortSetat = constrain(confortSetat + (id == 98 ? 0.5 : -0.5), 17.0, 25.0);
    char cmd[24];
    snprintf(cmd, sizeof(cmd), "TEMP CONFORT %d", (int)lroundf(confortSetat * 10));
    trimiteArduino(cmd);
    dateNoiUI = true;
  }
  if (id == 94) schimbaEcran(E_JURNAL);
  if (id == 99) calibreazaAtingerea();
}
