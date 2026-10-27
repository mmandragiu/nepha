// ui - ecranul tactil: bara de sus, filele de jos, cardurile si atingerile
//
// Fiecare ecran (Acasa, Camere, ...) are doua functii in tab-ul lui:
//   desenX(tot)  - tot = true: deseneaza tot ecranul; false: doar ce s-a schimbat
//   atingeX(id)  - s-a apasat butonul cu numarul id
// Butoanele se "inregistreaza" cu adaugaZona() cand sunt desenate.

#include "fonturi.h"

Ecran tft;
EcranUI ecranCurent = E_ACASA;
bool redesenTot = true;
bool dateNoiUI = false;
bool arataSugestie = false;
bool animatie = false;          // ceva de pe ecran e in miscare: mai desenam un cadru peste ~30 ms
uint32_t ultimaAtingere = 0;
uint32_t ultimDesen = 0;
uint32_t ultimCadru = 0;
uint32_t ultimPuls = 0;
int16_t punctX = -1, punctY = 0;   // punctul din pastila "Smart", care pulseaza
uint8_t luminaEcran = 0;
uint16_t minutAfisat = 0xFFFF;

struct Zona {
  int16_t x, y, w, h;
  uint8_t id;
};
Zona zone[32];
uint8_t nrZone = 0;
String semnaturi[16];
MarimeFont fontCurent = (MarimeFont)255;

const char *const NUME_FILE[6] = {"Acasă", "Camere", "Energie", "Rutină", "Grădină", "Alarmă"};
const char *const TITLURI[9] = {"Acasă", "Camere", "Energie", "Ce a învățat casa", "Grădina", "Alarma", "Jurnalul casei", "Setări", "Alarma"};

// ---------------------------------------------------------------- unelte de desen

void font(MarimeFont f) {
  if (f == fontCurent) return;
  const uint8_t *date[] = {font_mic, font_text, font_titlu, font_mare};
  tft.loadFont(date[f]);
  fontCurent = f;
}

void card(int x, int y, int w, int h, uint16_t culoare) {
  // colturile si umbra le stergem intai, altfel marginile netede se ingroasa la fiecare redesenare
  tft.fillRect(x, y, 14, 14, C_FUNDAL);
  tft.fillRect(x + w - 14, y, 14, 14, C_FUNDAL);
  tft.fillRect(x, y + h - 14, 14, 16, C_FUNDAL);
  tft.fillRect(x + w - 14, y + h - 14, 14, 16, C_FUNDAL);
  tft.fillRect(x + 14, y + h, w - 28, 2, C_FUNDAL);
  // umbra: doua linii, una mai inchisa si una mai deschisa, doar sub card
  tft.fillSmoothRoundRect(x, y + h - 26, w, 28, 14, C_UMBRA2);
  tft.fillSmoothRoundRect(x, y + h - 27, w, 28, 14, C_UMBRA);
  tft.fillSmoothRoundRect(x, y, w, h, 14, C_MARGINE);
  tft.fillSmoothRoundRect(x + 1, y + 1, w - 2, h - 2, 13, culoare);
}

void scrie(const String &text, int x, int y, MarimeFont f, uint16_t culoare, uint16_t fundal, textdatum_t aliniere) {
  font(f);
  tft.setTextDatum(aliniere);
  tft.setTextColor(culoare, fundal);
  tft.drawString(text.c_str(), x, y);
}

// Text pe mai multe randuri, rupt la cuvinte. Intoarce cate randuri a scris.
uint8_t paragraf(const String &text, int x, int y, int latime, MarimeFont f, uint16_t culoare,
                 uint16_t fundal, uint8_t maxRanduri) {
  font(f);
  tft.setTextDatum(textdatum_t::top_left);
  tft.setTextColor(culoare, fundal);
  int inaltime = tft.fontHeight() + 2;
  uint8_t randuri = 0;
  String rand;
  int i = 0, n = text.length();
  while (i < n && randuri < maxRanduri) {
    int spatiu = text.indexOf(' ', i);
    if (spatiu < 0) spatiu = n;
    String cuvant = text.substring(i, spatiu);
    String incercare = rand.length() ? rand + " " + cuvant : cuvant;
    if (tft.textWidth(incercare.c_str()) > latime && rand.length()) {
      if (randuri == maxRanduri - 1) {          // ultimul rand si mai e text: punem "..."
        while (rand.length() && tft.textWidth((rand + "…").c_str()) > latime) rand.remove(rand.length() - 1);
        rand += "…";
        tft.drawString(rand.c_str(), x, y + randuri * inaltime);
        return randuri + 1;
      }
      tft.drawString(rand.c_str(), x, y + randuri * inaltime);
      randuri++;
      rand = cuvant;
    } else {
      rand = incercare;
    }
    i = spatiu + 1;
  }
  if (rand.length() && randuri < maxRanduri) {
    tft.drawString(rand.c_str(), x, y + randuri * inaltime);
    randuri++;
  }
  return randuri;
}

void pastila(const String &text, int x, int y, uint16_t culoare, uint16_t fundal, textdatum_t aliniere) {
  font(F_MIC);
  int w = tft.textWidth(text.c_str()) + 16, h = tft.fontHeight() + 6;
  if (aliniere == textdatum_t::top_right) x -= w;
  if (aliniere == textdatum_t::top_center) x -= w / 2;
  tft.fillSmoothRoundRect(x, y, w, h, h / 2, fundal);
  tft.setTextDatum(textdatum_t::middle_center);
  tft.setTextColor(culoare, fundal);
  tft.drawString(text.c_str(), x + w / 2, y + h / 2 + 1);
}

void buton(const String &text, int x, int y, int w, int h, uint8_t id, bool principal) {
  if (principal) {
    tft.fillSmoothRoundRect(x, y, w, h, 12, C_VERDE);
    scrie(text, x + w / 2, y + h / 2 + 1, F_TEXT, C_CARD, C_VERDE, textdatum_t::middle_center);
  } else {
    tft.fillSmoothRoundRect(x, y, w, h, 12, C_MARGINE);
    tft.fillSmoothRoundRect(x + 1, y + 1, w - 2, h - 2, 11, C_CARD);
    scrie(text, x + w / 2, y + h / 2 + 1, F_TEXT, C_TEXT, C_CARD, textdatum_t::middle_center);
  }
  adaugaZona(x, y, w, h, id);
}

void bara(int x, int y, int w, int h, float procent, uint16_t culoare, uint16_t fundal) {
  procent = constrain(procent, 0, 100);
  tft.fillSmoothRoundRect(x, y, w, h, h / 2, fundal);
  int plin = w * procent / 100;
  if (plin >= h) tft.fillSmoothRoundRect(x, y, plin, h, h / 2, culoare);
  else if (plin > 0) tft.fillSmoothCircle(x + h / 2, y + h / 2, h / 2, culoare);
}

// 21.4 -> "21,4" (la noi se scrie cu virgula)
String zecimal(float x, uint8_t zecimale) {
  if (isnan(x)) return "–";
  String s(x, (unsigned int)zecimale);
  s.replace('.', ',');
  return s;
}

String grade(float x) {
  return isnan(x) ? String("–") : zecimal(x, 1) + "°";
}

void adaugaZona(int x, int y, int w, int h, uint8_t id) {
  Zona z = {(int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h, id};
  for (uint8_t i = 0; i < nrZone; i++)
    if (zone[i].id == id) {      // butonul exista deja (am redesenat doar o bucata de ecran)
      zone[i] = z;
      return;
    }
  if (nrZone < 32) zone[nrZone++] = z;
}

void stergeZona(uint8_t id) {
  for (uint8_t i = 0; i < nrZone; i++)
    if (zone[i].id == id) {
      zone[i] = zone[--nrZone];
      return;
    }
}

// true daca ce vrem sa desenam in "slot" e altceva decat ce e deja pe ecran
bool schimbat(uint8_t slot, const String &semnatura) {
  if (slot >= 16 || semnaturi[slot] == semnatura) return false;
  semnaturi[slot] = semnatura;
  return true;
}

uint16_t amesteca(uint16_t a, uint16_t b, uint8_t cat) {
  uint8_t r = (((a >> 11) & 31) * (255 - cat) + ((b >> 11) & 31) * cat) / 255;
  uint8_t g = (((a >> 5) & 63) * (255 - cat) + ((b >> 5) & 63) * cat) / 255;
  uint8_t bl = ((a & 31) * (255 - cat) + (b & 31) * cat) / 255;
  return (r << 11) | (g << 5) | bl;
}

// Curba cubic-bezier(0.4, 0, 0.2, 1), aceeasi ca pe pagina web: porneste hotarat si se
// opreste bland. t = cat a trecut din animatie (0..1), intoarce cat s-a miscat (0..1).
float usor(float t) {
  if (t <= 0) return 0;
  if (t >= 1) return 1;
  float a = 0, b = 1, u = t;
  for (uint8_t i = 0; i < 14; i++) {      // cautam u pentru care x(u) = t, injumatatind
    u = (a + b) / 2;
    float x = 3 * (1 - u) * (1 - u) * u * 0.4 + 3 * (1 - u) * u * u * 0.2 + u * u * u;
    if (x < t) a = u;
    else b = u;
  }
  return 3 * (1 - u) * u * u + u * u * u;
}

// ---------------------------------------------------------------- iconite (~24 px)

void iconCasa(int cx, int cy, uint16_t c, uint16_t fundal) {
  tft.fillTriangle(cx - 12, cy - 1, cx, cy - 12, cx + 12, cy - 1, c);
  tft.fillRect(cx - 8, cy - 2, 17, 13, c);
  tft.fillRect(cx - 2, cy + 4, 5, 7, fundal);      // usa
}

void iconBec(int cx, int cy, uint16_t c, uint16_t fundal) {
  tft.fillSmoothCircle(cx, cy - 3, 8, c);
  tft.fillRect(cx - 4, cy + 4, 9, 4, c);
  tft.fillRect(cx - 3, cy + 9, 7, 2, c);
  tft.fillSmoothCircle(cx - 3, cy - 5, 2, fundal);   // sclipire
}

void iconFulger(int cx, int cy, uint16_t c) {
  tft.fillTriangle(cx + 3, cy - 12, cx - 7, cy + 2, cx + 1, cy + 2, c);
  tft.fillTriangle(cx - 1, cy - 2, cx + 7, cy - 2, cx - 3, cy + 12, c);
}

void iconRutina(int cx, int cy, uint16_t c) {
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      tft.fillSmoothRoundRect(cx - 11 + i * 8, cy - 11 + j * 8, 6, 6, 2, (i + j) % 2 ? amesteca(c, C_CARD, 150) : c);
}

void iconPicatura(int cx, int cy, uint16_t c) {
  tft.fillSmoothCircle(cx, cy + 4, 7, c);
  tft.fillTriangle(cx - 6, cy + 1, cx + 6, cy + 1, cx, cy - 12, c);
}

void iconScut(int cx, int cy, uint16_t c, uint16_t fundal) {
  tft.fillSmoothRoundRect(cx - 9, cy - 11, 18, 13, 3, c);
  tft.fillTriangle(cx - 9, cy, cx + 9, cy, cx, cy + 12, c);
  tft.fillSmoothCircle(cx, cy - 3, 3, fundal);      // gaura de cheie
  tft.fillRect(cx - 1, cy - 2, 3, 6, fundal);
}

void iconSoare(int cx, int cy, uint16_t c, uint16_t fundal) {
  for (int k = 0; k < 8; k++) {
    float u = k * PI / 4;
    tft.drawWideLine(cx + cos(u) * 8, cy + sin(u) * 8, cx + cos(u) * 12, cy + sin(u) * 12, 2, c);
  }
  tft.fillSmoothCircle(cx, cy, 6, c);
}

void iconVant(int cx, int cy, uint16_t c) {
  tft.drawWideLine(cx - 11, cy - 5, cx + 6, cy - 5, 2, c);
  tft.drawWideLine(cx - 11, cy + 1, cx + 10, cy + 1, 2, c);
  tft.drawWideLine(cx - 11, cy + 7, cx + 3, cy + 7, 2, c);
}

void iconRotita(int cx, int cy, uint16_t c, uint16_t fundal) {
  for (int k = 0; k < 8; k++) {
    float u = k * PI / 4;
    tft.fillSmoothCircle(cx + cos(u) * 8, cy + sin(u) * 8, 3, c);
  }
  tft.fillSmoothCircle(cx, cy, 8, c);
  tft.fillSmoothCircle(cx, cy, 3, fundal);
}

void iconWifi(int cx, int cy, uint16_t c) {
  tft.fillArc(cx, cy + 7, 11, 13, 225, 315, c);
  tft.fillArc(cx, cy + 7, 6, 8, 225, 315, c);
  tft.fillSmoothCircle(cx, cy + 6, 2, c);
}

// ---------------------------------------------------------------- barele de sus si de jos

static uint16_t minutDeAfisat() {
  struct tm t;
  if (!casa.test && areOra && getLocalTime(&t, 0)) return t.tm_hour * 60 + t.tm_min;
  return casa.minut;
}

static void desenOra() {
  uint16_t m = minutDeAfisat();
  minutAfisat = m;
  tft.fillRect(330, 6, 104, 32, C_FUNDAL);
  scrie(oraText(m), 424, 22, F_TEXT, C_TEXT, C_FUNDAL, textdatum_t::middle_right);
  // Wi-Fi: inchis = internet, galben = doar reteaua NEPHA-casa, gri deschis = nimic
  iconWifi(350, 21, online() ? C_TEXT : (modAP() ? C_SOARE : C_MARGINE));
}

// pastila cu modul casei; in modul smart are un punct verde care "respira"
static void desenMod() {
  punctX = -1;
  if (casa.test) {
    pastila("TEST", 300, 12, C_CARD, C_SOARE, textdatum_t::top_right);
    return;
  }
  if (!casa.primit) return;
  const char *t = casa.mod ? "Smart" : "Clasic";
  uint16_t c = casa.mod ? C_VERDE : C_TEXT2, f = casa.mod ? C_VERDE_D : C_GRI_D;
  font(F_MIC);
  int w = tft.textWidth(t) + 30, h = tft.fontHeight() + 6, x = 300 - w;
  tft.fillSmoothRoundRect(x, 12, w, h, h / 2, f);
  tft.fillSmoothCircle(x + 12, 12 + h / 2, 3, c);
  scrie(t, x + 20, 12 + h / 2 + 1, F_MIC, c, f, textdatum_t::middle_left);
  if (casa.mod) {
    punctX = x + 12;
    punctY = 12 + h / 2;
  }
}

// o data la ~2 secunde punctul se stinge aproape de tot si se aprinde la loc
static void puls() {
  if (punctX < 0) return;
  float u = (millis() % 2000) / 2000.0;
  uint8_t k = 255 - (uint8_t)(180 * (0.5 - 0.5 * cos(u * 2 * PI)));
  tft.fillSmoothCircle(punctX, punctY, 4, C_VERDE_D);
  tft.fillSmoothCircle(punctX, punctY, 3, amesteca(C_VERDE_D, C_VERDE, k));
}

static void desenBaraSus() {
  tft.fillRect(0, 0, 480, SUS, C_FUNDAL);
  scrie(TITLURI[ecranCurent], 16, 23, F_TITLU, C_TEXT, C_FUNDAL, textdatum_t::middle_left);
  desenMod();
  desenOra();
  iconRotita(456, 22, C_TEXT2, C_FUNDAL);
  adaugaZona(436, 0, 44, SUS, 110);
}

static void desenFile() {
  tft.fillRect(0, 272, 480, 48, C_CARD);
  tft.drawFastHLine(0, 272, 480, C_MARGINE);
  EcranUI activ = ecranCurent == E_PIN ? E_ALARMA : ecranCurent;
  for (uint8_t i = 0; i < 6; i++) {
    int cx = 40 + i * 80;
    bool deschisa = i == activ;
    uint16_t c = deschisa ? C_VERDE : C_TEXT2, f = deschisa ? C_VERDE_D : C_CARD;
    // fila deschisa sta pe o pastila verde deschis, cu iconita si numele in verde
    if (deschisa) tft.fillSmoothRoundRect(cx - 36, 274, 72, 44, 12, f);
    switch (i) {
      case 0: iconCasa(cx, 288, c, f); break;
      case 1: iconBec(cx, 288, c, f); break;
      case 2: iconFulger(cx, 288, c); break;
      case 3: iconRutina(cx, 288, c); break;
      case 4: iconPicatura(cx, 288, c); break;
      case 5: iconScut(cx, 288, c, f); break;
    }
    scrie(NUME_FILE[i], cx, 309, F_MIC, c, f, textdatum_t::middle_center);
    adaugaZona(i * 80, 272, 80, 48, 100 + i);
  }
}

// ---------------------------------------------------------------- ecrane

static void deseneaza(bool tot) {
  switch (ecranCurent) {
    case E_ACASA: desenAcasa(tot); break;
    case E_CAMERE: desenCamere(tot); break;
    case E_ENERGIE: desenEnergie(tot); break;
    case E_RUTINA: desenRutina(tot); break;
    case E_GRADINA: desenGradina(tot); break;
    case E_ALARMA: desenAlarma(tot); break;
    case E_JURNAL: desenJurnal(tot); break;
    case E_SETARI: desenSetari(tot); break;
    case E_PIN: desenPin(tot); break;
  }
}

// "Casa pare goala, armez alarma?" - peste ecranul curent
static void desenSugestie() {
  nrZone = 0;
  tft.fillRect(0, SUS, 480, 272 - SUS, C_FUNDAL);
  card(40, 60, 400, 196);
  iconScut(76, 98, C_VERDE, C_CARD);
  scrie("Casa pare goală", 100, 98, F_TITLU, C_TEXT, C_CARD, textdatum_t::middle_left);
  paragraf("Nu e nimeni acasă și nu se întoarce nimeni curând. Armez alarma?", 64, 124, 352, F_MIC, C_TEXT2, C_CARD, 3);
  buton("Nu acum", 64, 200, 168, 40, 121);
  buton("Armează", 248, 200, 168, 40, 120, true);
}

void schimbaEcran(EcranUI e) {
  ecranCurent = e;
  redesenTot = true;
}

static void redeseneaza() {
  nrZone = 0;
  for (uint8_t i = 0; i < 16; i++) semnaturi[i] = "";
  tft.startWrite();
  tft.fillRect(0, SUS, 480, 272 - SUS, C_FUNDAL);
  desenBaraSus();
  desenFile();
  deseneaza(true);
  if (arataSugestie) desenSugestie();
  tft.endWrite();
  redesenTot = false;
}

static void apasa(uint8_t id) {
  if (arataSugestie) {
    if (id == 120) trimiteArduino("ALARMA PLECAT");
    if (id == 120 || id == 121) {
      arataSugestie = false;
      redesenTot = true;
    }
    return;
  }
  if (id >= 100 && id < 106) {
    schimbaEcran((EcranUI)(id - 100));
    return;
  }
  if (id == 110) {
    schimbaEcran(E_SETARI);
    return;
  }
  switch (ecranCurent) {
    case E_ACASA: atingeAcasa(id); break;
    case E_CAMERE: atingeCamere(id); break;
    case E_ENERGIE: atingeEnergie(id); break;
    case E_RUTINA: atingeRutina(id); break;
    case E_GRADINA: atingeGradina(id); break;
    case E_ALARMA: atingeAlarma(id); break;
    case E_JURNAL: atingeJurnal(id); break;
    case E_SETARI: atingeSetari(id); break;
    case E_PIN: atingePin(id); break;
  }
}

static void verificaAtingere() {
  static bool apasat = false;
  static uint32_t ultimaApasare = 0;
  uint16_t x, y;
  bool acum = tft.getTouch(&x, &y);
  if (acum && !apasat && millis() - ultimaApasare > 180) {
    ultimaApasare = millis();
    bool doarTrezire = luminaEcran < 200;      // ecranul era stins: prima atingere doar il aprinde
    ultimaAtingere = millis();
    if (!doarTrezire) {
      for (int8_t i = nrZone - 1; i >= 0; i--) {   // de la ultima zona: popup-urile sunt deasupra
        Zona &z = zone[i];
        if (x >= z.x && x < z.x + z.w && y >= z.y && y < z.y + z.h) {
          bip(2400, 12);
          apasa(z.id);
          break;
        }
      }
    }
  }
  apasat = acum;
}

// lumina ecranului: scade dupa un minut fara atingere (si mai mult noaptea)
static void ajusteazaLumina() {
  struct tm t;
  bool noapte = areOra && getLocalTime(&t, 0) && (t.tm_hour >= 23 || t.tm_hour < 6);
  uint8_t dorit = millis() - ultimaAtingere < 60000 ? 255 : (noapte ? 12 : 60);
  if (casa.alarma >= 4) dorit = 255;
  if (dorit != luminaEcran) {
    luminaEcran = dorit;
    tft.setBrightness(dorit);
  }
}

void calibreazaAtingerea() {
  tft.fillScreen(C_FUNDAL);
  scrie("Calibrăm ecranul", 240, 130, F_TITLU, C_TEXT, C_FUNDAL, textdatum_t::middle_center);
  scrie("Atinge vârful fiecărei săgeți, pe rând", 240, 170, F_TEXT, C_TEXT2, C_FUNDAL, textdatum_t::middle_center);
  uint16_t cal[8];
  tft.calibrateTouch(cal, C_VERDE, C_FUNDAL, 24);
  Preferences p;
  p.begin("nepha", false);
  p.putBytes("touch", cal, sizeof(cal));
  p.end();
  redesenTot = true;
}

void uiInit() {
  tft.init();
  tft.setRotation(1);
  tft.setBrightness(255);
  luminaEcran = 255;
  tft.fillScreen(C_FUNDAL);
  iconCasa(240, 118, C_VERDE, C_FUNDAL);
  scrie("NEPHA", 240, 160, F_TITLU, C_TEXT, C_FUNDAL, textdatum_t::middle_center);
  scrie("casa care se gândește singură", 240, 192, F_TEXT, C_TEXT2, C_FUNDAL, textdatum_t::middle_center);

  // daca tinem degetul pe ecran la pornire, calibram din nou
  delay(800);
  uint16_t cal[8], x, y;
  Preferences p;
  p.begin("nepha", true);
  size_t n = p.getBytes("touch", cal, sizeof(cal));
  p.end();
  if (n != sizeof(cal) || tft.getTouch(&x, &y)) calibreazaAtingerea();
  else tft.setTouchCalibrate(cal);
  ultimaAtingere = millis();
}

void uiDateNoi() {
  dateNoiUI = true;
}

void uiEveniment(const EvenimentP &e) {
  if (e.cod == 15) {                 // sugestie: armam alarma?
    arataSugestie = true;
    redesenTot = true;
  }
  if (e.cod == 13) {                 // alarma
    if (e.a == 4) deschidePin();
    else if (e.a == 5 && ecranCurent != E_PIN) schimbaEcran(E_ALARMA);
    else if (ecranCurent == E_PIN && e.a == 0) schimbaEcran(E_ALARMA);
    if (e.a >= 2) arataSugestie = false;
    ultimaAtingere = millis();
  }
  if (e.cod == 16 && ecranCurent == E_PIN) pinRaspuns(e);
  dateNoiUI = true;
}

void uiPas() {
  verificaAtingere();
  ajusteazaLumina();
  if (redesenTot) {
    redeseneaza();
    return;
  }
  uint32_t m = millis();
  bool date = dateNoiUI && m - ultimDesen > 400;     // a venit ceva nou de la Arduino
  bool cadru = animatie && m - ultimCadru >= 30;     // sau e ceva in miscare pe ecran
  if ((date || cadru) && !arataSugestie) {
    if (date) {
      dateNoiUI = false;
      ultimDesen = m;
    }
    ultimCadru = m;
    animatie = false;
    tft.startWrite();
    deseneaza(false);
    if (date) {
      if (schimbat(14, String(online()) + modAP()) || minutDeAfisat() != minutAfisat) desenOra();
      if (schimbat(15, String(casa.mod) + casa.test + casa.primit)) desenBaraSus();
    }
    tft.endWrite();
  }
  if (m - ultimPuls >= 50) {
    ultimPuls = m;
    tft.startWrite();
    puls();
    tft.endWrite();
  }
}
