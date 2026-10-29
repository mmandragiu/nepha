// panou - vorbim cu ESP32 (ecranul) si cu Serial Monitor
//
// Arduino trimite o linie de stare pe secunda:
//     S h=452 m=1 t=21.4,20.1,22.0 te=8.5 ...
// si o linie pentru fiecare lucru important pe care il face (jurnalul casei):
//     E <cod> <a> <b>
// Panoul trimite comenzi simple, una pe linie, de exemplu:
//     ORA 1290            VREME 80 16 40 85         ALARMA PLECAT
// Aceleasi comenzi merg si din Serial Monitor (115200 baud, "Newline").
// Lista completa: docs/protocol.md

// pe ce vorbim cu panoul se alege in config.h (LEGATURA)
#if !defined(FARA_PANOU)
LegaturaI2C legaturaPanou;        // I2C, vezi legatura.h
#endif

char liniePanou[40];
uint8_t lungPanou = 0;
#if !defined(FARA_PANOU)
char linieSerial[40];
uint8_t lungSerial = 0;
#endif
uint32_t ultimaStare = 0;

void panouInit() {
#if defined(FARA_PANOU)
  Wire.begin();
#else
  legaturaPanou.begin(ADRESA_ARDUINO);
#endif
}

static void citesteLinii(Stream &s, char *linie, uint8_t &lung, Print &raspuns) {
  while (s.available()) {
    char ch = s.read();
    if (ch == '\r') continue;
    if (ch == '\n') {
      linie[lung] = 0;
      if (lung > 0) executaComanda(linie, raspuns);
      lung = 0;
    } else if (lung < 39) {
      linie[lung++] = ch;
    }
  }
}

static void scrieTemp(Print &p, float t) {
  if (isnan(t)) p.print('-');
  else p.print(t, 1);
}

static void scrieOra(Print &p) {
  uint16_t m = minutulZilei();
  if (m / 60 < 10) p.print('0');
  p.print(m / 60);
  p.print(':');
  if (m % 60 < 10) p.print('0');
  p.print(m % 60);
}

// " cheie=valoare" (cheia e un text din PROGMEM)
static void camp(Print &p, const char *cheie, int valoare) {
  p.print((const __FlashStringHelper *)cheie);
  p.print(valoare);
}

static void campF(Print &p, const char *cheie, float valoare, uint8_t zecimale) {
  p.print((const __FlashStringHelper *)cheie);
  if (isnan(valoare)) p.print('-');
  else p.print(valoare, zecimale);
}

// Ce inseamna fiecare camp e scris in docs/protocol.md
void trimiteStare(Print &p) {
  Energie &e = casa.energie;
  uint8_t prezenta = 0, ferestre = 0;
  for (uint8_t i = 0; i < NR_CAMERE; i++) {
    if (casa.camere[i].prezenta) prezenta |= 1 << i;
    if (casa.camere[i].fereastra) ferestre |= 1 << i;
  }
  camp(p, PSTR("S h="), minutulZilei());
  camp(p, PSTR(" m="), casa.mod);
  camp(p, PSTR(" sc="), stareScenariu());
  p.print(F(" t="));
  for (uint8_t i = 0; i < NR_CAMERE; i++) {
    if (i) p.print(',');
    scrieTemp(p, casa.camere[i].temp);
  }
  campF(p, PSTR(" te="), !isnan(casa.tempAfara) || !casa.vreme.valida ? casa.tempAfara : casa.vreme.tempAfara, 1);
  camp(p, PSTR(" ua="), casa.umiditateAer);
  camp(p, PSTR(" ln="), casa.luminaNaturala);
  camp(p, PSTR(" p="), prezenta);
  p.print(F(" l="));
  for (uint8_t i = 0; i < NR_CAMERE; i++) {
    if (i) p.print(',');
    p.print(casa.camere[i].luminaTinta);
  }
  camp(p, PSTR(" f="), ferestre);
  camp(p, PSTR(" i="), casa.incalzire.pornita);
  campF(p, PSTR(" ti="), casa.incalzire.tinta, 1);
  camp(p, PSTR(" tc="), setari.confort);
  camp(p, PSTR(" mi="), casa.incalzire.motiv);
  campF(p, PSTR(" rt="), casa.incalzire.rata, 1);
  camp(p, PSTR(" b="), (int)e.soc);
  campF(p, PSTR(" ps="), e.pSolar, 2);
  campF(p, PSTR(" pv="), e.pVant, 2);
  campF(p, PSTR(" pc="), e.pConsum, 2);
  camp(p, PSTR(" e="), e.mod);
  camp(p, PSTR(" a="), casa.alarma.stare);
  camp(p, PSTR(" u="), casa.usiDeschise);
  camp(p, PSTR(" sol="), casa.gradina.umiditate);
  camp(p, PSTR(" pm="), casa.gradina.pompa);
  camp(p, PSTR(" pr="), casa.priza.pornita);
  campF(p, PSTR(" wh="), e.whLumini + e.whIncalzire + e.whPompa + e.whPriza, 0);
  campF(p, PSTR(","), e.whClasic, 0);
  campF(p, PSTR(" mwh="), e.mwhMasurat, 0);
  camp(p, PSTR(" ok="), casa.senzori);
  p.println();
}

static void scrieEveniment(Print &p, uint8_t cod, uint8_t a, uint8_t b) {
  p.print(F("E "));
  p.print(cod);
  p.print(' ');
  p.print(a);
  p.print(' ');
  p.println(b);
}

void trimiteEveniment(uint8_t cod, uint8_t a, uint8_t b) {
  if (stareScenariu() == 2) return;     // in testul rapid nu umplem jurnalul
#if !defined(FARA_PANOU)
  scrieEveniment(LEGATURA, cod, a, b);
#endif

  // si pe Serial, cu ora (ce inseamna fiecare cod: docs/protocol.md, in cuvinte pe panou).
  // Numele evenimentelor nu mai incap: pe unele placi Uno raman doar 30 KB pentru program.
  Serial.print('[');
  scrieOra(Serial);
  Serial.print(F("] "));
  scrieEveniment(Serial, cod, a, b);
}

// pentru panou: C mod lumini incalzire pompa priza total mWh_masurat litri
// (valorile sunt in ordinea din RezultatTest, asa ca le luam pe rand)
void trimiteRezultatTest(uint8_t mod) {
  const float *v = &rezultate[mod].lumini;
  LEGATURA.print(F("C "));
  LEGATURA.print(mod);
  for (uint8_t k = 0; k < 7; k++) {
    LEGATURA.print(' ');
    LEGATURA.print(v[k], 0);
  }
  LEGATURA.println();
}

static bool este(const char *a, const char *textPgm) {
  return a && strcasecmp_P(a, textPgm) == 0;
}

void executaComanda(char *linie, Print &raspuns) {
  char *cmd = strtok(linie, " ");
  char *a1 = strtok(NULL, " ");
  char *a2 = strtok(NULL, " ");
  char *a3 = strtok(NULL, " ");
  char *a4 = strtok(NULL, " ");
  if (!cmd) return;

  if (este(cmd, PSTR("ORA"))) {
    if (a1 && !inScenariu()) ceasSeteaza(atoi(a1));
  } else if (este(cmd, PSTR("VREME"))) {
    if (a4 && !inScenariu()) {
      casa.vreme.ploaie = atoi(a1);
      casa.vreme.oraPloaie = atoi(a2);
      casa.vreme.soare = atoi(a3);
      casa.vreme.tempAfara = atoi(a4) / 10.0;
      casa.vreme.valida = true;
      casa.vreme.primitaLa = millis();
    }
  } else if (este(cmd, PSTR("MOD"))) {
    ModCasa nou = este(a1, PSTR("CLASIC")) ? CLASIC : SMART;
    if (nou != casa.mod && !inScenariu()) {
      casa.mod = nou;
      setari.mod = nou;
      salveazaSetari();
      trimiteEveniment(EV_MOD, nou, 0);
    }
  } else if (este(cmd, PSTR("ALARMA"))) {
    if (este(a1, PSTR("ACASA"))) alarmaArmeaza(ALARMA_ACASA);
    else if (este(a1, PSTR("PLECAT"))) alarmaArmeaza(ALARMA_PLECAT);
    else if (este(a1, PSTR("OPRIT")) && a2) alarmaDezarmeaza(atoi(a2));
  } else if (este(cmd, PSTR("PIN"))) {
    if (a2 && (uint16_t)atoi(a1) == setari.pin) {
      setari.pin = atoi(a2);
      salveazaSetari();
      raspuns.println(F("# ok"));
    }
  } else if (este(cmd, PSTR("TEMP"))) {
    if (a2) {
      int16_t v = constrain(atoi(a2), 100, 280);
      if (este(a1, PSTR("CONFORT"))) setari.confort = v;
      else if (este(a1, PSTR("NOAPTE"))) setari.noapte = v;
      else if (este(a1, PSTR("ECO"))) setari.eco = v;
      else if (este(a1, PSTR("PLECAT"))) setari.plecat = v;
      salveazaSetari();
    }
  } else if (este(cmd, PSTR("LUMINA"))) {
    uint8_t i = a1 ? atoi(a1) : 255;
    if (i < NR_CAMERE && a2) {
      Camera &c = casa.camere[i];
      if (este(a2, PSTR("AUTO"))) c.manual = -1;
      else {
        c.manual = constrain(atoi(a2), 0, 100);
        c.manualPanaLa = acum() + 3600;     // respectam alegerea o ora
      }
    }
  } else if (este(cmd, PSTR("PRIZA"))) {
    if (este(a1, PSTR("AUTO"))) casa.priza.manual = -1;
    else if (a1) casa.priza.manual = atoi(a1) ? 1 : 0;
  } else if (este(cmd, PSTR("UDA"))) {
    pornestePompa(MU_MANUAL);
  } else if (este(cmd, PSTR("PLOAIE"))) {
    if (a1) {
      casa.vreme.ploua = atoi(a1);
      casa.vreme.plouaLa = millis();
    }
  } else if (este(cmd, PSTR("LIVOLO"))) {
    if (este(a1, PSTR("LEAGA"))) livoloLeaga();
  } else if (este(cmd, PSTR("INVATA"))) {
    if (a1) usiInvata(atoi(a1));
  } else if (este(cmd, PSTR("STERGE"))) {
    if (a1) usiSterge(atoi(a1));
  } else if (este(cmd, PSTR("USA"))) {
    if (a2) usiSeteaza(atoi(a1), atoi(a2));
  } else if (este(cmd, PSTR("TEST"))) {
    if (este(a1, PSTR("RAPID"))) testRapid(raspuns);
    else if (este(a1, PSTR("STOP"))) scenariuOpreste();
    else if (este(a1, PSTR("CLASIC"))) scenariuPorneste(CLASIC);
    else if (este(a1, PSTR("SMART"))) scenariuPorneste(SMART);
  } else if (este(cmd, PSTR("RUTINA"))) {
    if (este(a1, PSTR("STERGE"))) rutinaStergeTot();
    else rutinaTrimite(raspuns);
  } else if (este(cmd, PSTR("SCAN"))) {
    senzoriScan();
  } else if (este(cmd, PSTR("STARE"))) {
    trimiteStare(Serial);
  } else {
    raspuns.print(F("# ? "));
    raspuns.println(cmd);
  }
}

void panouPas() {
  citesteLinii(LEGATURA, liniePanou, lungPanou, LEGATURA);
#if !defined(FARA_PANOU)
  citesteLinii(Serial, linieSerial, lungSerial, Serial);
#endif
  if (millis() - ultimaStare >= PERIOADA_STARE) {
    ultimaStare = millis();
    trimiteStare(LEGATURA);
  }
}
