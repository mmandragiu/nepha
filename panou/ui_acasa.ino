// ui_acasa - primul ecran: cum e in casa si afara, bateria, economia de azi
// si ultimul lucru pe care l-a facut casa (cu motivul lui).

// Ecranul are trei randuri, cu 8 px intre ele: doua carduri mari (in casa, afara),
// trei mici (baterie, economie, alarma) si o fasie cu ultimul lucru facut de casa.
#define R1_Y 50
#define R1_H 110
#define R2_Y 168
#define R2_H 58
#define R3_Y 234
#define R3_H 34

// randul din mijloc: cel din mijloc e mai lat (are si leii)
static const int COL_X[3] = {12, 150, 328};
static const int COL_L[3] = {130, 170, 140};

// Cand se schimba starea unui card (porneste incalzirea, se armeaza alarma, vine un eveniment
// nou), ce s-a schimbat apare treptat, in 200 ms. O temperatura care urca cu o zecime nu are
// trecere: se scrie direct.
static String stareCard[6];
static uint32_t trecereDe[6];
static uint8_t opac = 255;

static uint8_t trecere(uint8_t i, const String &stare) {
  if (stareCard[i] != stare) {
    // prima data (cand intram pe ecran) desenam direct, fara trecere
    trecereDe[i] = stareCard[i].length() ? millis() : millis() - 200;
    stareCard[i] = stare;
  }
  uint32_t t = millis() - trecereDe[i];
  if (t >= 200) return 255;
  animatie = true;
  return 255 * usor(t / 200.0);
}

// culoarea c, cat se vede acum din ea (in timpul unei treceri)
static uint16_t ap(uint16_t c, uint16_t fundal = C_CARD) {
  return amesteca(fundal, c, opac);
}

static void cardInterior() {
  uint8_t insigna = casa.motiv == 2 || casa.motiv == 6 ? casa.motiv : 0;
  String stare = String(casa.primit) + casa.incalzire + insigna;
  opac = trecere(0, stare);
  if (!schimbat(0, zecimal(tempCasaP(), 1) + casa.tinta + stare + opac)) return;
  card(12, R1_Y, 224, R1_H);
  scrie("În casă", 28, R1_Y + 12, F_MIC, C_TEXT2, C_CARD);
  if (!casa.primit) {
    paragraf("Aștept datele de la Arduino...", 28, R1_Y + 44, 190, F_TEXT, C_TEXT2, C_CARD, 2);
    return;
  }
  // insigna sta pe randul etichetei, deasupra temperaturii, ca sa nu se bata cu cifrele
  if (insigna == 2) pastila("preîncălzire", 220, R1_Y + 9, ap(C_VERDE), ap(C_VERDE_D), textdatum_t::top_right);
  if (insigna == 6) pastila("geam deschis", 220, R1_Y + 9, ap(C_CALD), ap(C_CALD_D), textdatum_t::top_right);
  scrie(grade(tempCasaP()), 26, R1_Y + 56, F_MARE, C_TEXT, C_CARD, textdatum_t::middle_left);
  uint16_t c = ap(casa.incalzire ? C_CALD : C_TEXT2);
  tft.fillSmoothCircle(32, R1_Y + 92, 4, c);
  String text = casa.incalzire ? "încălzește spre " : "țintă ";
  scrie(text + grade(casa.tinta), 42, R1_Y + 92, F_MIC, c, C_CARD, textdatum_t::middle_left);
}

static void cardAfara() {
  bool ploua = vreme.valida && vreme.oraPloaie != 255;
  String stare = String(vreme.valida) + ploua + (vreme.soare > 40) + online();
  opac = trecere(1, stare);
  String semn = zecimal(casa.tempAfara, 1) + zecimal(vreme.temp, 1) + vreme.ploaie + vreme.oraPloaie + stare + opac;
  if (!schimbat(1, semn)) return;
  card(244, R1_Y, 224, R1_H);
  scrie("Afară", 260, R1_Y + 12, F_MIC, C_TEXT2, C_CARD);
  if (vreme.valida) {
    if (ploua) iconPicatura(444, R1_Y + 21, ap(C_APA));
    else iconSoare(444, R1_Y + 21, ap(vreme.soare > 40 ? C_SOARE : C_TEXT2), C_CARD);
  }
  float t = !isnan(casa.tempAfara) ? casa.tempAfara : (vreme.valida ? vreme.temp : NAN);
  scrie(grade(t), 258, R1_Y + 56, F_MARE, C_TEXT, C_CARD, textdatum_t::middle_left);
  String linie;
  if (!vreme.valida) linie = online() ? "aștept prognoza..." : "fără internet, fără prognoză";
  else if (ploua) linie = "ploaie " + String(vreme.ploaie) + "% pe la " + String(vreme.oraPloaie) + ":00";
  else linie = vreme.soare > 40 ? "senin în următoarele ore" : "fără ploaie, înnorat";
  scrie(linie, 260, R1_Y + 92, F_MIC, ap(C_TEXT2), C_CARD, textdatum_t::middle_left);
}

static void cardMic(int i, const String &eticheta, const String &valoare, uint16_t culoare) {
  int x = COL_X[i];
  card(x, R2_Y, COL_L[i], R2_H);
  scrie(eticheta, x + 14, R2_Y + 10, F_MIC, C_TEXT2, C_CARD);
  scrie(valoare, x + 14, R2_Y + 37, F_TEXT, culoare, C_CARD, textdatum_t::middle_left);
}

// Bara bateriei nu sare la noua valoare: aluneca pana acolo in 0,5 s.
static float barPe = -1, barDe = 0;
static uint8_t barSpre = 0;
static uint32_t barStart = 0;

static float baraBaterie() {
  uint8_t tinta = casa.primit ? casa.baterie : 0;
  if (barPe < 0) {                 // prima data: fara animatie
    barPe = barDe = barSpre = tinta;
    barStart = millis() - 500;
  }
  if (tinta != barSpre) {          // a venit alt procent: pornim de unde e bara acum
    barDe = barPe;
    barSpre = tinta;
    barStart = millis();
  }
  uint32_t t = millis() - barStart;
  if (t >= 500) barPe = barSpre;
  else {
    barPe = barDe + (barSpre - barDe) * usor(t / 500.0);
    animatie = true;
  }
  return barPe;
}

static void cardBaterie() {
  float pe = baraBaterie();
  opac = trecere(2, String(casa.modEnergie) + casa.primit);
  // bara are 52 px: o redesenam doar cand se misca macar un pixel
  String semn = String(casa.baterie) + casa.modEnergie + casa.primit + opac + "|" + int(pe * 52 / 100);
  if (!schimbat(2, semn)) return;
  uint16_t c = casa.modEnergie == 3 ? C_ROSU : (casa.modEnergie == 2 ? C_SOARE : C_VERDE);
  cardMic(0, "Baterie", casa.primit ? String(casa.baterie) + "%" : "–", C_TEXT);
  bara(COL_X[0] + 64, R2_Y + 32, 52, 10, pe, ap(c, C_GRI_D), C_GRI_D);
}

static void cardEconomie() {
  float kwh = (casa.whClasic - casa.whAzi) / 1000.0;
  if (isnan(kwh) || kwh < 0) kwh = 0;
  opac = trecere(3, String(casa.mod));
  if (!schimbat(3, zecimal(kwh, 1) + casa.mod + opac)) return;
  if (!casa.mod) {
    cardMic(1, "Economie azi", "mod clasic", ap(C_TEXT2));
    return;
  }
  cardMic(1, "Economie azi", zecimal(kwh, 1) + " kWh", ap(C_VERDE));
  scrie(zecimal(kwh * PRET_KWH, 1) + " lei", COL_X[1] + COL_L[1] - 14, R2_Y + 10, F_MIC, ap(C_TEXT2), C_CARD,
        textdatum_t::top_right);
}

// pe cardul mic incape doar un cuvant; eticheta "Alarma" e deja deasupra
static const char *const ALARMA_SCURT[6] = {"Dezarmată", "Se armează", "Noapte", "Plecat", "Cere PIN", "Sună!"};

static void cardAlarma() {
  opac = trecere(4, String(casa.alarma));
  if (!schimbat(4, String(casa.alarma) + opac)) return;
  uint16_t c = casa.alarma >= 4 ? C_ROSU : (casa.alarma == 0 ? C_TEXT2 : C_VERDE);
  cardMic(2, "Alarmă", casa.alarma < 6 ? ALARMA_SCURT[casa.alarma] : "", ap(c));
  adaugaZona(COL_X[2], R2_Y, COL_L[2], R2_H, 1);
}

// fasia de jos, chiar deasupra filelor: ultimul lucru facut de casa
static void cardAcum() {
  String stare = nrJurnal ? String(jurnal[0].minut) + jurnal[0].cod + jurnal[0].a : String("-");
  opac = trecere(5, stare);
  if (!schimbat(5, stare + opac)) return;
  int mijloc = R3_Y + R3_H / 2;
  card(12, R3_Y, 456, R3_H);
  if (!nrJurnal) {
    scrie("Aici apare ce decide casa, și de ce.", 28, mijloc, F_MIC, ap(C_TEXT2), C_CARD, textdatum_t::middle_left);
  } else {
    scrie(oraText(jurnal[0].minut), 28, mijloc, F_MIC, ap(C_VERDE), C_CARD, textdatum_t::middle_left);
    font(F_MIC);
    int x = 28 + tft.textWidth("00:00") + 10;
    paragraf(textEveniment(jurnal[0]), x, mijloc - 9, 456 - (x - 12) - 34, F_MIC, ap(C_TEXT), C_CARD, 1);
  }
  scrie("›", 452, mijloc - 1, F_TITLU, C_TEXT2, C_CARD, textdatum_t::middle_right);
  adaugaZona(12, R3_Y, 456, R3_H, 2);
}

void desenAcasa(bool tot) {
  if (tot) {
    for (uint8_t i = 0; i < 6; i++) stareCard[i] = "";
    adaugaZona(12, R1_Y, 224, R1_H, 3);      // interior -> Camere
    adaugaZona(244, R1_Y, 224, R1_H, 4);     // afara -> Gradina
    adaugaZona(12, R2_Y, 308, R2_H, 5);      // baterie, economie -> Energie
  }
  cardInterior();
  cardAfara();
  cardBaterie();
  cardEconomie();
  cardAlarma();
  cardAcum();
  opac = 255;
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
