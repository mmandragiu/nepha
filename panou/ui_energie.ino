// ui_energie - de unde vine curentul acum, in ce mod e casa, si "ziua de test":
// aceeasi zi traita ca o casa clasica si ca o casa NEPHA, cu energia comparata.

static const char *explicatieEnergie(uint8_t mod) {
  switch (mod) {
    case 1: return "Baterie plină: pornim ce poate aștepta.";
    case 2: return "Lumini mai slabe, un grad mai puțin.";
    case 3: return "Doar strictul necesar.";
  }
  return "Energie destulă.";
}

static void cardAcumEnergie() {
  String semn = zecimal(casa.solar, 2) + zecimal(casa.vant, 2) + zecimal(casa.consum, 2) + casa.baterie +
                casa.modEnergie + zecimal(productieEstimata(6), 1);
  if (!schimbat(0, semn)) return;
  card(12, 50, 232, 216);
  scrie("Acum", 28, 62, F_MIC, C_TEXT2, C_CARD);
  uint16_t c = casa.modEnergie == 3 ? C_ROSU : (casa.modEnergie == 2 ? C_SOARE : C_VERDE);
  uint16_t fundal = casa.modEnergie == 3 ? C_ROSU_D : (casa.modEnergie == 2 ? C_SOARE_D : C_VERDE_D);
  pastila(String("Mod ") + textEnergie(casa.modEnergie), 228, 59, c, fundal, textdatum_t::top_right);

  iconSoare(40, 100, C_SOARE, C_CARD);
  scrie(zecimal(casa.solar, 2) + " W", 60, 100, F_TEXT, C_TEXT, C_CARD, textdatum_t::middle_left);
  iconVant(40, 130, C_APA);
  scrie(zecimal(casa.vant, 2) + " W", 60, 130, F_TEXT, C_TEXT, C_CARD, textdatum_t::middle_left);
  iconCasa(40, 160, C_TEXT2, C_CARD);
  scrie("casa consumă " + zecimal(casa.consum, 2) + " W", 60, 160, F_MIC, C_TEXT2, C_CARD, textdatum_t::middle_left);

  // bateria, desenata ca o baterie
  int bx = 140, by = 95;
  uint16_t plinCuloare = amesteca(c, C_CARD, 120);
  tft.fillSmoothRoundRect(bx, by, 80, 40, 8, C_MARGINE);
  tft.fillSmoothRoundRect(bx + 2, by + 2, 76, 36, 6, C_CARD);
  tft.fillSmoothRoundRect(bx + 80, by + 13, 6, 14, 2, C_MARGINE);
  int plin = 72 * constrain(casa.baterie, 0, 100) / 100;
  if (plin > 6) tft.fillSmoothRoundRect(bx + 4, by + 4, plin, 32, 4, plinCuloare);
  // procentul sta in intregime fie pe partea plina, fie pe cea goala (are fundalul ei)
  String procent = String(casa.baterie) + "%";
  font(F_MIC);
  if (plin >= tft.textWidth(procent.c_str()) + 8)
    scrie(procent, bx + 4 + plin / 2, by + 21, F_MIC, C_TEXT, plinCuloare, textdatum_t::middle_center);
  else
    scrie(procent, bx + 4 + plin + (72 - plin) / 2, by + 21, F_MIC, C_TEXT, C_CARD, textdatum_t::middle_center);

  paragraf(explicatieEnergie(casa.modEnergie), 28, 184, 204, F_MIC, C_TEXT, C_CARD, 2);
  float estimat = productieEstimata(6);
  if (!isnan(estimat))
    scrie("Soare în 6 ore: ~" + zecimal(estimat, 1) + " Wh", 28, 252, F_MIC, C_TEXT2, C_CARD, textdatum_t::middle_left);
}

static void randTest(const char *nume, float wh, float procent, int y, uint16_t culoare) {
  scrie(nume, 266, y, F_MIC, culoare, C_CARD, textdatum_t::middle_left);
  scrie(zecimal(wh / 1000, 1) + " kWh", 452, y, F_MIC, culoare, C_CARD, textdatum_t::middle_right);
  bara(266, y + 12, 186, 8, procent, culoare, C_GRI_D);
}

static void cardTest() {
  RezultatTest &c = rezultate[0], &s = rezultate[1];
  String semn = String(c.valid) + s.valid + c.total + s.total + casa.test;
  if (!schimbat(1, semn)) return;
  for (uint8_t id = 30; id <= 32; id++) stergeZona(id);
  card(252, 50, 216, 216);
  scrie("Ziua de test", 266, 62, F_MIC, C_TEXT2, C_CARD);

  if (casa.test == 1) {
    scrie("Rulează pe machetă", 266, 92, F_TEXT, C_TEXT, C_CARD);
    paragraf("O zi întreagă în ~6 minute.", 266, 118, 186, F_MIC, C_TEXT2, C_CARD, 4);
    buton("Oprește", 262, 224, 196, 34, 32);
    return;
  }
  if (c.valid && s.valid && c.total > 0) {
    float economie = (c.total - s.total) * 100 / c.total;
    scrie("−" + zecimal(economie, 0) + "%", 266, 98, F_TITLU, C_VERDE, C_CARD, textdatum_t::middle_left);
    scrie("energie", 346, 92, F_MIC, C_TEXT2, C_CARD, textdatum_t::middle_left);
    scrie("într-o zi", 346, 108, F_MIC, C_TEXT2, C_CARD, textdatum_t::middle_left);
    randTest("Clasic", c.total, 100, 130, C_TEXT2);
    randTest("NEPHA", s.total, s.total * 100 / c.total, 162, C_VERDE);
  } else {
    paragraf("Clasic vs NEPHA: aceeași zi, aceeași vreme.", 266, 88, 186, F_MIC, C_TEXT2, C_CARD, 3);
  }
  buton("Calculează", 262, 190, 196, 32, 30, true);
  buton("Pe machetă (6 min)", 262, 228, 196, 32, 31);
}

void desenEnergie(bool tot) {
  cardAcumEnergie();
  cardTest();
}

void atingeEnergie(uint8_t id) {
  if (id == 30) trimiteArduino("TEST RAPID");
  if (id == 31) trimiteArduino("TEST SMART");
  if (id == 32) trimiteArduino("TEST STOP");
}
