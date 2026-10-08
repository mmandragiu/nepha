// legatura - mesajele schimbate cu Arduino (descrise in docs/protocol.md)
//
// De la Arduino vin:
//   S ...        starea casei, o data pe secunda
//   E cod a b    s-a intamplat ceva (intra in jurnal)
//   C ...        rezultatul zilei de test
//   R / Z        rutina invatata
//   # ...        mesaje pentru oameni (le scriem doar pe Serial)
// Noi trimitem comenzi text: ORA, VREME, ALARMA ..., MOD ..., etc.

char linieArduino[256];
uint16_t lungArduino = 0;
uint32_t ultimaOraTrimisa = 0;
uint32_t ultimaVremeTrimisa = 0;
uint32_t ultimaRutinaCeruta = 0;
bool trebuieSincronizat = true;     // Arduino tocmai a pornit: ii trimitem ora si vremea

void legaturaInit() {
  Serial2.begin(VITEZA_ARDUINO, SERIAL_8N1, PIN_RX_ARDUINO, PIN_TX_ARDUINO);
}

void trimiteArduino(const char *comanda) {
  Serial2.println(comanda);
  Serial.print("-> ");
  Serial.println(comanda);
}

// "21.4,20.1,-" -> numere; "-" inseamna ca Arduino nu stie valoarea
static void listaNumere(char *text, float *rezultat, uint8_t cate) {
  for (uint8_t i = 0; i < cate; i++) {
    char *dupa;
    float x = strtod(text, &dupa);
    rezultat[i] = dupa == text ? NAN : x;
    text = strchr(text, ',');
    if (!text) break;
    text++;
  }
}

static float numar(const char *text) {
  char *dupa;
  float x = strtod(text, &dupa);
  return dupa == text ? NAN : x;
}

static void citesteStare(char *linie) {
  CasaP c = casa;
  float valori[NR_CAMERE];
  for (char *bucata = strtok(linie + 2, " "); bucata; bucata = strtok(NULL, " ")) {
    char *egal = strchr(bucata, '=');
    if (!egal) continue;
    *egal = 0;
    const char *k = bucata;
    char *v = egal + 1;
    int n = atoi(v);
    if (!strcmp(k, "h")) c.minut = n;
    else if (!strcmp(k, "m")) c.mod = n;
    else if (!strcmp(k, "sc")) c.test = n;
    else if (!strcmp(k, "t")) {
      listaNumere(v, valori, NR_CAMERE);
      for (uint8_t i = 0; i < NR_CAMERE; i++) c.camere[i].temp = valori[i];
    } else if (!strcmp(k, "te")) c.tempAfara = numar(v);
    else if (!strcmp(k, "ln")) c.luminaNat = n;
    else if (!strcmp(k, "p")) for (uint8_t i = 0; i < NR_CAMERE; i++) c.camere[i].prezenta = n & (1 << i);
    else if (!strcmp(k, "l")) {
      listaNumere(v, valori, NR_CAMERE);
      for (uint8_t i = 0; i < NR_CAMERE; i++) c.camere[i].lumina = isnan(valori[i]) ? 0 : valori[i];
    } else if (!strcmp(k, "f")) for (uint8_t i = 0; i < NR_CAMERE; i++) c.camere[i].fereastra = n & (1 << i);
    else if (!strcmp(k, "i")) c.incalzire = n;
    else if (!strcmp(k, "ti")) c.tinta = numar(v);
    else if (!strcmp(k, "mi")) c.motiv = n;
    else if (!strcmp(k, "rt")) c.rata = numar(v);
    else if (!strcmp(k, "b")) c.baterie = n;
    else if (!strcmp(k, "ps")) c.solar = numar(v);
    else if (!strcmp(k, "pv")) c.vant = numar(v);
    else if (!strcmp(k, "pc")) c.consum = numar(v);
    else if (!strcmp(k, "e")) c.modEnergie = n;
    else if (!strcmp(k, "a")) c.alarma = n;
    else if (!strcmp(k, "u")) c.usi = n;
    else if (!strcmp(k, "sol")) c.sol = n;
    else if (!strcmp(k, "pm")) c.pompa = n;
    else if (!strcmp(k, "pr")) c.priza = n;
    else if (!strcmp(k, "wh")) {
      float doua[2];
      listaNumere(v, doua, 2);
      c.whAzi = doua[0];
      c.whClasic = doua[1];
    } else if (!strcmp(k, "mwh")) c.mwh = numar(v);
    else if (!strcmp(k, "ok")) c.senzori = n;
  }
  bool inceputTest = c.test == 1 && casa.test != 1;
  c.primit = true;
  c.primitLa = millis();
  casa = c;
  if (inceputTest) nrIstoric = 0;    // graficul arata ziua de test de la inceput
  uiDateNoi();
}

static void adaugaInJurnal(uint8_t cod, uint8_t a, uint8_t b) {
  memmove(&jurnal[1], &jurnal[0], sizeof(EvenimentP) * (NR_JURNAL - 1));
  jurnal[0] = {casa.minut, cod, a, b};
  if (nrJurnal < NR_JURNAL) nrJurnal++;
  Serial.print("[jurnal] ");
  Serial.println(textEveniment(jurnal[0]));
  uiEveniment(jurnal[0]);
}

static void citesteLinie(char *linie) {
  switch (linie[0]) {
    case 'S':
      citesteStare(linie);
      break;
    case 'E': {
      int cod = 0, a = 0, b = 0;
      if (sscanf(linie + 2, "%d %d %d", &cod, &a, &b) >= 1) {
        if (cod == 1) trebuieSincronizat = true;    // Arduino a pornit din nou
        adaugaInJurnal(cod, a, b);
      }
      break;
    }
    case 'C': {
      int mod;
      RezultatTest r;
      if (sscanf(linie + 2, "%d %f %f %f %f %f %f %f", &mod, &r.lumini, &r.incalzire, &r.pompa, &r.priza,
                 &r.total, &r.mwh, &r.litri) == 8 && (mod == 0 || mod == 1)) {
        r.valid = true;
        rezultate[mod] = r;
        uiDateNoi();
      }
      break;
    }
    case 'R': {
      int camera = atoi(linie + 2);
      char *v = strchr(linie + 2, ' ');
      if (v && camera >= 0 && camera < NR_CAMERE) {
        float p[24];
        listaNumere(v + 1, p, 24);
        for (uint8_t h = 0; h < 24; h++) rutina.p[camera][h] = isnan(p[h]) ? 0 : p[h];
      }
      break;
    }
    case 'Z': {
      float z[24];
      listaNumere(linie + 2, z, 24);
      for (uint8_t h = 0; h < 24; h++) rutina.zile[h] = isnan(z[h]) ? 0 : z[h];
      rutina.valida = true;
      uiDateNoi();
      break;
    }
    default:
      Serial.print("Arduino: ");
      Serial.println(linie);
  }
}

static void trimiteOra() {
  struct tm t;
  if (!getLocalTime(&t, 0)) return;
  char cmd[16];
  snprintf(cmd, sizeof(cmd), "ORA %d", t.tm_hour * 60 + t.tm_min);
  trimiteArduino(cmd);
}

static void trimiteVreme() {
  char cmd[40];
  snprintf(cmd, sizeof(cmd), "VREME %d %d %d %d", vreme.ploaie, vreme.oraPloaie, vreme.soare, (int)lroundf(vreme.temp * 10));
  trimiteArduino(cmd);
}

void legaturaPas() {
  while (Serial2.available()) {
    char ch = Serial2.read();
    if (ch == '\r') continue;
    if (ch == '\n') {
      linieArduino[lungArduino] = 0;
      if (lungArduino) citesteLinie(linieArduino);
      lungArduino = 0;
    } else if (lungArduino < sizeof(linieArduino) - 1) {
      linieArduino[lungArduino++] = ch;
    }
  }

  // nu-i trimitem nimic lui Arduino in timpul zilei de test (are ceasul lui)
  if (casa.test) return;
  uint32_t m = millis();
  if (areOra && (trebuieSincronizat || !ultimaOraTrimisa || m - ultimaOraTrimisa > 60000UL)) {
    ultimaOraTrimisa = m;
    trimiteOra();
  }
  if (vreme.valida && (trebuieSincronizat || !ultimaVremeTrimisa || m - ultimaVremeTrimisa > 600000UL)) {
    ultimaVremeTrimisa = m;
    trimiteVreme();
  }
  if (casa.primit && (trebuieSincronizat || !ultimaRutinaCeruta || m - ultimaRutinaCeruta > 600000UL)) {
    ultimaRutinaCeruta = m;
    trimiteArduino("RUTINA");
  }
  if (casa.primit) trebuieSincronizat = false;
}
