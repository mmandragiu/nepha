// ui_acasa - primul ecran: cum e in casa si afara, bateria, economia de azi
// si ultimul lucru pe care l-a facut casa (cu motivul lui).

// randul de jos are trei carduri; cel din mijloc e mai lat (are si leii)
static const int COL_X[3] = {12, 150, 328};
static const int COL_L[3] = {130, 170, 140};

static void cardInterior() {
  String semn = zecimal(tempCasaP(), 1) + casa.tinta + casa.motiv + casa.incalzire + casa.primit;
  if (!schimbat(0, semn)) return;
  card(12, 50, 224, 104);
  scrie("În casă", 28, 62, F_MIC, C_TEXT2, C_CARD);
  if (!casa.primit) {
    paragraf("Aștept datele de la Arduino...", 28, 90, 190, F_TEXT, C_TEXT2, C_CARD, 2);
    return;
  }
  scrie(grade(tempCasaP()), 26, 104, F_MARE, C_TEXT, C_CARD, textdatum_t::middle_left);
  uint16_t c = casa.incalzire ? C_CALD : C_TEXT2;
  tft.fillSmoothCircle(32, 140, 4, c);
  String stare = casa.incalzire ? "încălzește spre " : "țintă ";
  scrie(stare + grade(casa.tinta), 42, 140, F_MIC, c, C_CARD, textdatum_t::middle_left);
  if (casa.motiv == 2 || casa.motiv == 6) pastila(casa.motiv == 2 ? "preîncălzire" : "geam deschis", 222, 60,
                                                  casa.motiv == 2 ? C_VERDE : C_CALD,
                                                  casa.motiv == 2 ? C_VERDE_D : C_CALD_D, textdatum_t::top_right);
}

static void cardAfara() {
  String semn = zecimal(casa.tempAfara, 1) + vreme.valida + vreme.ploaie + vreme.oraPloaie + vreme.soare;
  if (!schimbat(1, semn)) return;
  card(244, 50, 224, 104);
  scrie("Afară", 260, 62, F_MIC, C_TEXT2, C_CARD);
  float t = !isnan(casa.tempAfara) ? casa.tempAfara : (vreme.valida ? vreme.temp : NAN);
  scrie(grade(t), 258, 104, F_MARE, C_TEXT, C_CARD, textdatum_t::middle_left);
  String linie;
  if (!vreme.valida) linie = online() ? "aștept prognoza..." : "fără internet, fără prognoză";
  else if (vreme.oraPloaie != 255) linie = "ploaie " + String(vreme.ploaie) + "% pe la " + String(vreme.oraPloaie) + ":00";
  else linie = vreme.soare > 40 ? "senin în următoarele ore" : "fără ploaie, înnorat";
  scrie(linie, 260, 140, F_MIC, C_TEXT2, C_CARD, textdatum_t::middle_left);
  if (vreme.valida) {
    if (vreme.oraPloaie != 255) iconPicatura(444, 74, C_APA);
    else iconSoare(444, 74, vreme.soare > 40 ? C_SOARE : C_TEXT2, C_CARD);
  }
}

static void cardMic(int i, const String &eticheta, const String &valoare, uint16_t culoare) {
  int x = COL_X[i];
  card(x, 162, COL_L[i], 56);
  scrie(eticheta, x + 14, 172, F_MIC, C_TEXT2, C_CARD);
  scrie(valoare, x + 14, 198, F_TEXT, culoare, C_CARD, textdatum_t::middle_left);
}

static void cardBaterie() {
  if (!schimbat(2, String(casa.baterie) + casa.modEnergie + casa.primit)) return;
  uint16_t c = casa.modEnergie == 3 ? C_ROSU : (casa.modEnergie == 2 ? C_SOARE : C_VERDE);
  cardMic(0, "Baterie", casa.primit ? String(casa.baterie) + "%" : "–", C_TEXT);
  bara(COL_X[0] + 64, 194, 52, 10, casa.baterie, c, C_GRI_D);
}

static void cardEconomie() {
  float kwh = (casa.whClasic - casa.whAzi) / 1000.0;
  if (isnan(kwh) || kwh < 0) kwh = 0;
  if (!schimbat(3, zecimal(kwh, 1) + casa.mod)) return;
  if (!casa.mod) {
    cardMic(1, "Economie azi", "mod clasic", C_TEXT2);
    return;
  }
  cardMic(1, "Economie azi", zecimal(kwh, 1) + " kWh", C_VERDE);
  scrie(zecimal(kwh * PRET_KWH, 1) + " lei", COL_X[1] + COL_L[1] - 14, 172, F_MIC, C_TEXT2, C_CARD, textdatum_t::top_right);
}

static void cardAlarma() {
  if (!schimbat(4, String(casa.alarma))) return;
  uint16_t c = casa.alarma >= 4 ? C_ROSU : (casa.alarma == 0 ? C_TEXT2 : C_VERDE);
  cardMic(2, "Alarmă", textAlarma(casa.alarma), c);
  adaugaZona(COL_X[2], 162, COL_L[2], 56, 1);
}

static void cardAcum() {
  String semn = nrJurnal ? String(jurnal[0].minut) + jurnal[0].cod + jurnal[0].a : String("-");
  if (!schimbat(5, semn)) return;
  card(12, 226, 456, 40);
  if (!nrJurnal) {
    scrie("Aici apare ce decide casa, și de ce.", 28, 246, F_MIC, C_TEXT2, C_CARD, textdatum_t::middle_left);
  } else {
    scrie(oraText(jurnal[0].minut), 28, 246, F_MIC, C_VERDE, C_CARD, textdatum_t::middle_left);
    font(F_MIC);
    int x = 28 + tft.textWidth("00:00") + 10;
    paragraf(textEveniment(jurnal[0]), x, 237, 456 - (x - 12) - 34, F_MIC, C_TEXT, C_CARD, 1);
  }
  scrie("›", 452, 245, F_TITLU, C_TEXT2, C_CARD, textdatum_t::middle_right);
  adaugaZona(12, 226, 456, 40, 2);
}

void desenAcasa(bool tot) {
  if (tot) {
    adaugaZona(12, 50, 224, 104, 3);    // interior -> Camere
    adaugaZona(244, 50, 224, 104, 4);   // afara -> Gradina
    adaugaZona(12, 162, 308, 56, 5);    // baterie, economie -> Energie
  }
  cardInterior();
  cardAfara();
  cardBaterie();
  cardEconomie();
  cardAlarma();
  cardAcum();
}

void atingeAcasa(uint8_t id) {
  switch (id) {
    case 1: schimbaEcran(E_ALARMA); break;
    case 2: schimbaEcran(E_JURNAL); break;
    case 3: schimbaEcran(E_CAMERE); break;
    case 4: schimbaEcran(E_GRADINA); break;
    case 5: schimbaEcran(E_ENERGIE); break;
  }
}

// media camerelor (doar cele pentru care avem temperatura)
float tempCasaP() {
  float suma = 0;
  uint8_t n = 0;
  for (uint8_t i = 0; i < NR_CAMERE; i++)
    if (!isnan(casa.camere[i].temp)) {
      suma += casa.camere[i].temp;
      n++;
    }
  return n ? suma / n : NAN;
}
