// rutina - partea care invata
//
// Pentru fiecare ora din zi tinem minte:
//   rutinaZile[ora]            - in cate zile am urmarit ora asta
//   rutinaOcupat[camera][ora]  - in cate dintre ele a fost cineva in camera (macar 10 minute)
//
// Probabilitatea ca la ora asta sa fie cineva in camera:
//        p = (ocupat + 1) / (zile + 2)
// +1 si +2 vin de la regula lui Laplace: fara nicio zi observata da 50% ("nu stiu"),
// iar cu fiecare zi se apropie de ce se intampla de fapt in casa.
//
// Ca sa tina pasul cand se schimba programul (vacanta, alt orar la scoala),
// la 30 de zile injumatatim numerele: zilele vechi conteaza tot mai putin.
//
// Din model scoatem doua lucruri:
//   - in cate minute vine cineva acasa (termostatul porneste inainte)
//   - cat sa astepte lumina dupa ultima miscare (luminile)

uint8_t rutinaZile[24];
uint8_t rutinaOcupat[NR_CAMERE][24];
uint16_t secundePrezenta[NR_CAMERE];
uint16_t secundeObservate = 0;
uint8_t oraObservata = 255;

static void inchideOra(uint8_t ora) {
  if (secundeObservate < 1800) return;     // am vazut prea putin din ora asta, n-o punem la socoteala
  if (rutinaZile[ora] >= 30) {
    rutinaZile[ora] = 15;
    for (uint8_t i = 0; i < NR_CAMERE; i++) rutinaOcupat[i][ora] = (rutinaOcupat[i][ora] + 1) / 2;
  }
  rutinaZile[ora]++;
  for (uint8_t i = 0; i < NR_CAMERE; i++)
    if (secundePrezenta[i] >= 600) rutinaOcupat[i][ora]++;
  if (stareScenariu() == 0) salveazaOraRutina(ora);
}

void rutinaPas(uint32_t dt) {
  // invatam doar cand stim ora adevarata (sau in ziua de test)
  bool invatam = ceasSincronizat || inScenariu();
  uint8_t ora = oraZilei();
  if (ora != oraObservata) {
    if (oraObservata != 255 && invatam) inchideOra(oraObservata);
    oraObservata = ora;
    secundeObservate = 0;
    for (uint8_t i = 0; i < NR_CAMERE; i++) secundePrezenta[i] = 0;
  }
  if (dt > 3600) dt = 3600;
  secundeObservate = min(3600UL, secundeObservate + dt);
  for (uint8_t i = 0; i < NR_CAMERE; i++)
    if (casa.camere[i].prezenta) secundePrezenta[i] = min(3600UL, secundePrezenta[i] + dt);
}

// ceasul a sarit (a venit ora de la panou): ora curenta nu mai e una "curata"
void rutinaOraNoua() {
  oraObservata = 255;
}

// 0..100 %
uint8_t probabilitate(uint8_t camera, uint8_t ora) {
  return (uint16_t)(rutinaOcupat[camera][ora] + 1) * 100 / (rutinaZile[ora] + 2);
}

uint8_t probabilitateCasa(uint8_t ora) {
  uint8_t maxim = 0;
  for (uint8_t i = 0; i < NR_CAMERE; i++) maxim = max(maxim, probabilitate(i, ora));
  return maxim;
}

// Peste cate minute ne asteptam sa fie cineva treaz acasa, cautand in urmatoarele
// maxMinute. Orele de somn le sarim (atunci e "acasa", dar nu are nevoie de confort).
// -1 = nu se asteapta pe nimeni.
int16_t minutePanaVineCineva(uint16_t maxMinute) {
  uint16_t m = minutulZilei();
  // ora curenta o sarim: daca e cineva acum, se vede deja la senzori.
  // Ne uitam la inceputul fiecarei ore care urmeaza.
  for (uint16_t d = 60 - m % 60; d <= maxMinute; d += 60) {
    uint8_t ora = ((m + d) / 60) % 24;
    if (eOraDeSomn(ora)) continue;
    if (rutinaZile[ora] > 0 && probabilitateCasa(ora) >= 60) return d;
  }
  return -1;
}

void rutinaIncarca() {
  for (uint8_t h = 0; h < 24; h++) {
    rutinaZile[h] = EEPROM.read(ADR_ZILE + h);
    for (uint8_t i = 0; i < NR_CAMERE; i++) rutinaOcupat[i][h] = EEPROM.read(ADR_OCUPAT + i * 24 + h);
  }
  oraObservata = 255;
}

void rutinaStergeRAM() {
  memset(rutinaZile, 0, sizeof(rutinaZile));
  memset(rutinaOcupat, 0, sizeof(rutinaOcupat));
  oraObservata = 255;
}

void rutinaStergeTot() {
  rutinaStergeRAM();
  for (uint8_t h = 0; h < 24; h++) salveazaOraRutina(h);
}

// tabelul pentru panou: "R camera p0,p1,...,p23" si "Z zile0,...,zile23"
void rutinaTrimite(Print &p) {
  for (uint8_t i = 0; i < NR_CAMERE; i++) {
    p.print(F("R "));
    p.print(i);
    p.print(' ');
    for (uint8_t h = 0; h < 24; h++) {
      if (h) p.print(',');
      p.print(probabilitate(i, h));
    }
    p.println();
  }
  p.print(F("Z "));
  for (uint8_t h = 0; h < 24; h++) {
    if (h) p.print(',');
    p.print(rutinaZile[h]);
  }
  p.println();
}
