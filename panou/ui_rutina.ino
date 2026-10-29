// ui_rutina - "ce a invatat casa": pentru fiecare camera si fiecare ora,
// cat de probabil e sa fie cineva acolo. Cu cat e mai verde, cu atat mai sigur.
// Din tabelul asta casa stie cand sa porneasca incalzirea mai devreme si cat
// sa tina lumina aprinsa dupa ce nu mai vede miscare.
// Fara senzori de miscare camerele invata toate acelasi lucru (dupa alarma), asa ca
// desenam un singur rand, "Acasa", cu cea mai mare sansa la fiecare ora.

#define GRILA_X 116
#define CELULA 14

void desenRutina(bool tot) {
  uint16_t suma = 0;
  for (uint8_t i = 0; i < NR_CAMERE; i++)
    for (uint8_t h = 0; h < 24; h++) suma += rutina.p[i][h] * (h + 1);
  uint8_t ora = casa.minut / 60;
  bool miscare = casa.senzori & 0x200;
  uint8_t randuri = miscare ? NR_CAMERE : 1;
  if (!schimbat(0, String(suma) + rutina.valida + ora + zecimal(casa.rata, 1) + miscare)) return;

  card(12, 50, 456, 216);
  if (!rutina.valida) {
    scrie("Când e lume prin casă", 28, 62, F_TEXT, C_TEXT, C_CARD);
    paragraf("Se încarcă...", 28, 96, 420, F_MIC, C_TEXT2, C_CARD, 2);
    return;
  }
  uint8_t zile = 0;
  for (uint8_t h = 0; h < 24; h++) zile = max(zile, rutina.zile[h]);
  scrie("Când e lume prin casă", 28, 62, F_TEXT, C_TEXT, C_CARD);
  scrie("învățat din " + String(zile) + (zile == 1 ? " zi" : " zile") + ", pe ore", 452, 66, F_MIC, C_TEXT2, C_CARD, textdatum_t::top_right);

  for (uint8_t i = 0; i < randuri; i++) {
    int y = 98 + i * 30;
    scrie(miscare ? NUME_CAMERE[i] : "Acasă", 28, y + 12, F_MIC, C_TEXT, C_CARD, textdatum_t::middle_left);
    for (uint8_t h = 0; h < 24; h++) {
      uint8_t p = rutina.p[i][h];
      if (!miscare)
        for (uint8_t j = 1; j < NR_CAMERE; j++) p = max(p, rutina.p[j][h]);
      uint16_t c = amesteca(C_GRI_D, C_VERDE, constrain((p - 10) * 255 / 80, 0, 255));
      tft.fillSmoothRoundRect(GRILA_X + h * CELULA, y, CELULA - 2, 24, 3, c);
    }
  }
  // ora de acum
  tft.drawRoundRect(GRILA_X + ora * CELULA - 2, 95, CELULA + 2, 30 * randuri + 2, 4, C_TEXT);
  for (uint8_t h = 0; h < 24; h += 6)
    scrie(String(h), GRILA_X + h * CELULA + CELULA / 2 - 1, 98 + 30 * randuri + 8, F_MIC, C_TEXT2, C_CARD, textdatum_t::top_center);
  if (!miscare)
    paragraf("După alarmă: armată „Plecat” = nu e nimeni acasă.", 28, 160, 420, F_MIC, C_TEXT2, C_CARD, 1);

  if (!isnan(casa.rata) && casa.rata > 0)
    paragraf("Se încălzește cu " + zecimal(casa.rata, 1) + " °C pe oră.",
             28, 216, 290, F_MIC, C_TEXT2, C_CARD, 2);
  buton("Uită rutina", 330, 218, 126, 36, 40);
}

void atingeRutina(uint8_t id) {
  if (id == 40) {
    trimiteArduino("RUTINA STERGE");
    trimiteArduino("RUTINA");
  }
}
