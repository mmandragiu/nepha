// ui_gradina - cat de umed e pamantul si ce a hotarat casa despre udat (si de ce);
// daca e pornita placa din gradina: ploua acum? ce zona se uda?

// arc intre doua unghiuri (grade, 0 = la ora 3, in sensul acelor de ceas); trece si peste 360
static void arc(int cx, int cy, int r0, int r1, float a0, float a1, uint16_t c) {
  if (a1 > 360) {
    tft.fillArc(cx, cy, r0, r1, a0, 360, c);
    tft.fillArc(cx, cy, r0, r1, 0, a1 - 360, c);
  } else {
    tft.fillArc(cx, cy, r0, r1, a0, a1, c);
  }
}

// cauta in jurnal ultima decizie despre gradina (udare sau udare amanata)
static int8_t ultimaDecizieGradina() {
  for (uint8_t i = 0; i < nrJurnal; i++)
    if (jurnal[i].cod == 10 || jurnal[i].cod == 11) return i;
  return -1;
}

void desenGradina(bool tot) {
  int8_t d = ultimaDecizieGradina();
  String semn = String(casa.sol) + casa.pompa + d + (d >= 0 ? jurnal[d].minut : 0) + vreme.ploaie + vreme.oraPloaie + casa.mod +
                gradina.aici + gradina.ploua + gradina.zona;
  if (!schimbat(0, semn)) return;

  // stanga: "ceasul" umiditatii
  card(12, 50, 200, 216);
  scrie("Pământul", 28, 62, F_MIC, C_TEXT2, C_CARD);
  int cx = 112, cy = 162;
  arc(cx, cy, 70, 56, 135, 405, C_GRI_D);
  float unghi = 135 + 270.0 * constrain(casa.sol, 0, 100) / 100;
  uint16_t c = casa.sol < 35 ? C_SOARE : C_APA;
  if (casa.sol > 0) arc(cx, cy, 70, 56, 135, unghi, c);
  float prag = (135 + 270.0 * 35 / 100) * PI / 180;       // pragul de udare (35%)
  tft.drawWideLine(cx + cos(prag) * 52, cy + sin(prag) * 52, cx + cos(prag) * 74, cy + sin(prag) * 74, 1.5, C_TEXT2);
  scrie(String(casa.sol) + "%", cx, cy - 4, F_TITLU, C_TEXT, C_CARD, textdatum_t::middle_center);
  scrie("umiditate", cx, cy + 22, F_MIC, C_TEXT2, C_CARD, textdatum_t::middle_center);
  if (gradina.zona) pastila("zona " + String(gradina.zona) + " din " + String(NR_ZONE), cx, 236, C_APA, C_APA_D, textdatum_t::top_center);
  else if (casa.pompa) pastila("se udă acum", cx, 236, C_APA, C_APA_D, textdatum_t::top_center);

  // dreapta: decizia
  card(220, 50, 248, 216);
  scrie("Ce a hotărât casa", 236, 62, F_MIC, C_TEXT2, C_CARD);
  if (d >= 0) scrie(oraText(jurnal[d].minut), 452, 62, F_MIC, C_VERDE, C_CARD, textdatum_t::top_right);
  String text;
  if (d >= 0) text = textEveniment(jurnal[d]);
  else if (!casa.mod) text = "Mod clasic: udă zilnic la 12, indiferent de vreme.";
  else if (casa.sol >= 35) text = "Pământul e umed. Nu ud.";
  else text = "Pământul e uscat. Aștept o oră bună.";
  paragraf(text, 236, 88, 220, F_TEXT, C_TEXT, C_CARD, 3);

  String ploaie = "Prognoza: necunoscută";
  if (gradina.aici && gradina.ploua) ploaie = "Plouă acum";
  else if (vreme.valida) {
    if (vreme.oraPloaie != 255) ploaie = "Ploaie: " + String(vreme.ploaie) + "% pe la " + String(vreme.oraPloaie) + ":00";
    else ploaie = "Fără ploaie în 12 ore";
  }
  iconPicatura(246, 196, C_APA);
  scrie(ploaie, 262, 196, F_MIC, C_TEXT2, C_CARD, textdatum_t::middle_left);
  buton("Udă acum", 230, 220, 228, 38, 50);
}

void atingeGradina(uint8_t id) {
  if (id == 50) trimiteArduino("UDA");
}
