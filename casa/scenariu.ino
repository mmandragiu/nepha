// scenariu - ziua de test
//
// Ca sa aratam cat economiseste casa, o punem sa traiasca de doua ori ACEEASI zi:
// o data ca o casa clasica si o data ca o casa NEPHA. Aceeasi familie, aceeasi
// vreme, aceleasi ore - diferenta de energie vine doar din decizii.
//
// Ziua (o zi de octombrie):
//   06:30  se trezesc, mic dejun in bucatarie; la 06:45 deschid geamul sa aeriseasca
//   08:00  pleaca toti (scoala, serviciu) si armeaza alarma
//   17:30  se intorc acasa
//   18:30  cina, 19:30 - 22:00 televizor
//   22:30  la culcare
//   Afara: 2 grade dimineata, 13 la pranz. Ploua intre 16 si 18 (se stie de dimineata).
//
// Inainte de zi, casa isi invata rutina din 7 zile asemanatoare (orele variaza
// putin de la o zi la alta, ca in realitate).
//
//   TEST CLASIC / TEST SMART - ziua trece in ~6 minute, macheta chiar se aprinde,
//                              iar INA219 masoara cat consuma macheta de-adevaratelea
//   TEST RAPID               - totul calculat pe loc (cateva secunde), fara iesiri

struct Interval {
  uint8_t camera;
  uint16_t de, pana;      // minute din zi
};

const Interval PROGRAM[] PROGMEM = {
  {CAMERA_DORMITOR,    0,  390},   // doarme pana la 06:30
  {2,                390,  465},   // bucatarie 06:30 - 07:45
  {0,                435,  480},   // sufragerie 07:15 - 08:00
  {0,               1050, 1350},   // sufragerie 17:30 - 22:30
  {2,               1110, 1170},   // bucatarie 18:30 - 19:30
  {CAMERA_DORMITOR, 1350, 1440},   // la culcare 22:30
};
const uint8_t NR_INTERVALE = sizeof(PROGRAM) / sizeof(PROGRAM[0]);

// pe ore, de la 00:00 la 23:00
const int8_t TEMP_AFARA[24] PROGMEM = {4, 3, 3, 2, 2, 2, 2, 3, 5, 7, 9, 11, 12, 13, 13, 12, 10, 8, 7, 6, 6, 5, 5, 4};
const int8_t LUMINA_AFARA[24] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 10, 35, 60, 75, 85, 90, 85, 75, 55, 25, 15, 0, 0, 0, 0, 0, 0};
const int8_t VANT_CW[24] PROGMEM = {10, 10, 8, 8, 6, 6, 8, 10, 12, 15, 15, 18, 20, 22, 25, 30, 45, 50, 35, 20, 15, 12, 10, 10};  // centi-wati

#define VITEZA_TEST 240     // o ora de test = 15 secunde

uint8_t stareTest = 0;      // 0 = nu, 1 = test pe macheta, 2 = test rapid
uint16_t ziTest = 0;
int16_t decalajDimineata = 0, decalajSeara = 0;
float solTest = 30;
RezultatTest rezultate[2];

// ce aveam inainte de test, ca sa revenim
ModCasa modInainte;
uint32_t ceasInainte, millisInainte;
float rataInainte;
StareAlarma alarmaInainte;
Vreme vremeInainte;

bool inScenariu() { return stareTest != 0; }
uint8_t stareScenariu() { return stareTest; }

static bool eOcupat(uint8_t camera, uint16_t minut) {
  for (uint8_t k = 0; k < NR_INTERVALE; k++) {
    Interval iv;
    memcpy_P(&iv, &PROGRAM[k], sizeof(iv));
    if (iv.camera != camera) continue;
    int16_t d = iv.de < 600 ? decalajDimineata : decalajSeara;
    int16_t de = iv.de == 0 ? 0 : iv.de + d;
    int16_t pana = iv.pana == 1440 ? 1440 : iv.pana + d;
    if ((int16_t)minut >= de && (int16_t)minut < pana) return true;
  }
  return false;
}

// valoarea din tabelul pe ore, cu trecere lina intre ore
static float dinTabel(const int8_t *tabel, uint16_t minut) {
  uint8_t h = minut / 60;
  float a = (int8_t)pgm_read_byte(&tabel[h]);
  float b = (int8_t)pgm_read_byte(&tabel[(h + 1) % 24]);
  return a + (b - a) * (minut % 60) / 60.0;
}

static uint16_t aleator(uint32_t &x) {
  x = x * 1103515245UL + 12345UL;
  return (x >> 16) & 0x7FFF;
}

// casa traieste `zile` zile obisnuite, doar ca sa-si invete rutina
static void invataRutina(uint8_t zile) {
  uint32_t samanta = 2026;    // mereu aceeasi, ca rezultatele sa se poata repeta
  for (uint8_t z = 0; z < zile; z++) {
    decalajDimineata = (int16_t)(aleator(samanta) % 41) - 20;   // +-20 minute
    decalajSeara = (int16_t)(aleator(samanta) % 61) - 30;       // +-30 minute
    for (uint16_t pas = 0; pas < 144; pas++) {                  // pasi de 10 minute
      uint16_t m = minutulZilei();
      for (uint8_t i = 0; i < NR_CAMERE; i++) casa.camere[i].prezenta = eOcupat(i, m);
      rutinaPas(600);
      ceasAvanseaza(600);
    }
  }
  decalajDimineata = decalajSeara = 0;
}

static void pregatesteZiua(ModCasa mod) {
  casa.mod = mod;
  ceasSecunde = (ceasSecunde / 86400UL + 1) * 86400UL;    // miezul noptii
  rutinaStergeRAM();
  invataRutina(7);
  ziTest = ziua();

  camereLaZero(20.5);
  tempGeaman = 20.5;
  Incalzire &inc = casa.incalzire;
  inc.pornita = false;
  inc.rata = 3.0;
  inc.motiv = 255;
  inc.preincalzirePanaLa = 0;
  casa.energie.soc = 55;
  casa.energie.mod = ENERGIE_NORMAL;
  reseteazaContoare();
  Gradina &g = casa.gradina;
  g.umiditate = 30;
  solTest = 30;
  g.pompa = false;
  g.ultimaUdare = 0;
  g.ziuaProgramator = g.ziuaAnuntPloaie = 0xFFFF;
  casa.priza.pornita = true;
  casa.priza.manual = -1;
  casa.priza.standbyDin = acum();
  casa.alarma.stare = ALARMA_OPRITA;
  casa.vreme.valida = true;
  casa.vreme.ploaie = 80;
  casa.vreme.oraPloaie = 16;
  casa.vreme.soare = 50;
}

static void tineMinte() {
  modInainte = casa.mod;
  ceasInainte = ceasSecunde;
  millisInainte = millis();
  rataInainte = casa.incalzire.rata;
  alarmaInainte = casa.alarma.stare;
  vremeInainte = casa.vreme;
}

static void revino() {
  stareTest = 0;
  ceasViteza = 1;
  ceasSecunde = ceasInainte + (millis() - millisInainte) / 1000;
  secundaDecizie = ceasSecunde;
  casa.mod = modInainte;
  rutinaIncarca();                    // rutina adevarata a casei, din EEPROM
  casa.incalzire.rata = rataInainte;
  casa.incalzire.pornita = false;
  casa.incalzire.preincalzirePanaLa = 0;
  tempGeaman = NAN;
  casa.alarma.stare = alarmaInainte;
  casa.vreme = vremeInainte;
  reseteazaContoare();     // contoarele de azi o iau de la capat dupa test (n-avem RAM sa le pastram)
  casa.priza.pornita = true;
  casa.priza.standbyDin = ceasSecunde;
  casa.gradina.pompa = false;
  casa.tempAfara = NAN;
  camereLaZero(NAN);                  // temperaturile le citim din nou de la senzori
}

static void pastreazaRezultat(uint8_t mod) {
  Energie &e = casa.energie;
  RezultatTest &r = rezultate[mod];
  r.lumini = e.whLumini;
  r.incalzire = e.whIncalzire;
  r.pompa = e.whPompa;
  r.priza = e.whPriza;
  r.total = e.whLumini + e.whIncalzire + e.whPompa + e.whPriza;
  r.mwh = e.mwhMasurat;
  r.litri = e.litri;
}

// Intrarile "virtuale" pentru momentul curent al zilei de test
void scenariuPas(uint32_t dt) {
  if (stareTest == 1 && ziua() != ziTest) {    // s-a terminat ziua
    pastreazaRezultat(casa.mod);
    trimiteRezultatTest(casa.mod);
    trimiteEveniment(EV_TEST, 2, casa.mod);
    revino();
    return;
  }

  uint16_t m = minutulZilei();
  bool somn = eOraDeSomn(m / 60);
  for (uint8_t i = 0; i < NR_CAMERE; i++) {
    Camera &c = casa.camere[i];
    c.prezenta = eOcupat(i, m);
    // cine doarme e acasa, dar nu se misca (senzorul PIR nu l-ar vedea)
    if (c.prezenta && !(somn && i == CAMERA_DORMITOR)) c.ultimaMiscare = acum();
  }
  casa.tempAfara = dinTabel(TEMP_AFARA, m);
  casa.luminaNaturala = dinTabel(LUMINA_AFARA, m);
  casa.energie.pSolar = PM_SOLAR_MAX * casa.luminaNaturala / 100.0;
  casa.energie.pVant = dinTabel(VANT_CW, m) / 100.0;
  casa.vreme.tempAfara = casa.tempAfara;
  if (m >= 1080) casa.vreme.ploaie = 10;          // ploaia a trecut

  // geamul din bucatarie, deschis la aerisit (bucataria nu are senzor pe geam)
  if (NR_CAMERE > 2) casa.camere[2].geamDeschis = m >= 405 && m < 435;
  // televizorul
  casa.priza.tvPornit = casa.priza.pornita && m >= 1170 && m < 1320;
  // pleaca toti la 8 si armeaza alarma, se intorc la 17:30
  if (m >= 480 && m < 1050) {
    if (casa.alarma.stare == ALARMA_OPRITA) alarmaArmeaza(ALARMA_PLECAT);
  } else if (casa.alarma.stare != ALARMA_OPRITA) {
    alarmaSchimba(ALARMA_OPRITA, 255);
  }

  // pamantul: se usuca peste zi, se uda cand ploua
  float ore = dt / 3600.0;
  if (casa.gradina.umiditate > solTest + 1) solTest = casa.gradina.umiditate;   // tocmai am udat
  if (m >= 960 && m < 1080) solTest += 12 * ore;
  else solTest -= (m >= 480 && m < 1080 ? 0.6 : 0.2) * ore;
  solTest = constrain(solTest, 0.0, 100.0);
  casa.gradina.umiditate = (uint8_t)solTest;
}

void scenariuPorneste(ModCasa mod) {
  if (inScenariu()) return;
  tineMinte();
  stareTest = 1;
  pregatesteZiua(mod);
  ceasViteza = VITEZA_TEST;
  secundaDecizie = ceasSecunde;
  trimiteEveniment(EV_TEST, 1, mod);
}

void scenariuOpreste() {
  if (stareTest != 1) return;
  trimiteEveniment(EV_TEST, 0, casa.mod);
  revino();
}

static void scrieRand(Print &p, const __FlashStringHelper *nume, float clasic, float smart) {
  p.print(nume);
  p.print('\t');
  p.print(clasic, 0);
  p.print('\t');
  p.print(smart, 0);
  p.print('\t');
  if (clasic > 0) {
    p.print((clasic - smart) * 100 / clasic, 0);
    p.print('%');
  }
  p.println();
}

void testRapid(Print &p) {
  if (inScenariu()) return;
  p.println(F("# Ziua de test: clasic si smart..."));
  tineMinte();
  stareTest = 2;
  for (uint8_t mod = 0; mod < 2; mod++) {
    pregatesteZiua((ModCasa)mod);
    for (uint16_t pas = 0; pas < 1440; pas++) {     // pasi de cate un minut
      scenariuPas(60);
      decide(60);
      contorizeaza(60, 0);
      ceasAvanseaza(60);
    }
    pastreazaRezultat(mod);
  }
  revino();

  RezultatTest &c = rezultate[0];
  RezultatTest &s = rezultate[1];
  p.println(F("#\t\tClasic\tSmart\tEconomie (Wh)"));
  scrieRand(p, F("# Lumini\t"), c.lumini, s.lumini);
  scrieRand(p, F("# Incalzire"), c.incalzire, s.incalzire);
  scrieRand(p, F("# Udare\t"), c.pompa, s.pompa);
  scrieRand(p, F("# Priza TV"), c.priza, s.priza);
  scrieRand(p, F("# TOTAL\t"), c.total, s.total);
  scrieRand(p, F("# Apa (l)"), c.litri, s.litri);
  trimiteRezultatTest(0);
  trimiteRezultatTest(1);
}
