// energie - de unde vine curentul si pe ce se duce
//
// Macheta are o baterie (18650) incarcata de panoul solar si de turbina eoliana.
// INA219 masoara productia si consumul. Din ele alegem modul de energie:
//   SURPLUS  - bateria e plina si inca produce: e momentul pentru consumatorii
//              care pot astepta (pompa de irigatii)
//   ECONOMIE - bateria scade si nu produce: lumini mai slabe, un grad mai putin
//   CRITIC   - doar strictul necesar
// Daca prognoza spune ca urmeaza ore fara soare, intram in economie mai devreme.
//
// Tot aici numaram energia folosita azi si o comparam cu ce ar fi consumat
// o casa clasica in aceleasi conditii.

// tensiunea unei celule Li-ion la 0%, 10%, ... 100% (aproximativ, in repaus)
const float TENSIUNE_CELULA[11] PROGMEM = {3.30, 3.50, 3.60, 3.70, 3.75, 3.80, 3.85, 3.92, 4.00, 4.08, 4.20};

float socDinTensiune(float v) {
  if (v <= 3.30) return 0;
  for (uint8_t i = 1; i < 11; i++) {
    float sus = pgm_read_float(&TENSIUNE_CELULA[i]);
    if (v <= sus) {
      float jos = pgm_read_float(&TENSIUNE_CELULA[i - 1]);
      return (i - 1) * 10 + 10 * (v - jos) / (sus - jos);
    }
  }
  return 100;
}

// cat consuma macheta acum, dupa ce e pornit (cand nu avem INA219 pe consum)
static float consumModel() {
  float p = 0;
  for (uint8_t i = 0; i < NR_CAMERE; i++) p += PM_BEC * casa.camere[i].luminaTinta / 100.0;
  if (casa.incalzire.pornita) p += PM_INCALZIRE;
  if (casa.gradina.pompa) p += PM_POMPA;
  if (casa.priza.pornita) p += casa.priza.tvPornit ? PM_TV : PM_STANDBY;
  return p;
}

static float vantVirtual = 0.15;
static uint16_t rafala = 7;
static bool socInitializat = false;

// Completam ce nu masuram: productie din lumina naturala, vant care mai bate
// si o baterie virtuala. In ziua de test totul e virtual (vine din scenariu).
void energieVirtuala(uint32_t dt) {
  Energie &e = casa.energie;
  bool test = inScenariu();
  if (!test) {
    if (!(casa.senzori & SENZ_SOLAR)) e.pSolar = PM_SOLAR_MAX * casa.luminaNaturala / 100.0;
    if (!(casa.senzori & SENZ_VANT)) {
      // vantul virtual se schimba putin cate putin (numere "la intamplare" facute simplu)
      rafala = rafala * 75 + 74;
      vantVirtual += ((int8_t)(rafala >> 8) % 11) * 0.003;
      vantVirtual = constrain(vantVirtual, 0.0, 0.5);
      e.pVant = vantVirtual;
    }
  }
  if (test || !(casa.senzori & SENZ_CONSUM)) e.pConsum = consumModel();

  if (!test && !isnan(e.vBaterie)) {
    float s = socDinTensiune(e.vBaterie / CELULE_SERIE);
    // tensiunea sare cand porneste ceva, asa ca filtram
    e.soc = socInitializat ? e.soc * 0.95 + s * 0.05 : s;
    socInitializat = true;
  } else {
    // baterie virtuala: ce intra minus ce iese
    e.soc += (e.pSolar + e.pVant - e.pConsum) * (dt / 3600.0) / BATERIE_WH * 100.0;
    e.soc = constrain(e.soc, 0.0, 100.0);
  }
}

static float productieMedie = 0, consumMediu = 0;

void decideEnergie(uint32_t dt) {
  Energie &e = casa.energie;
  // decidem pe medii de ~15 minute: un nor sau caloriferul care porneste
  // n-ar trebui sa schimbe modul casei
  float a = dt / (900.0 + dt);
  productieMedie += a * (e.pSolar + e.pVant - productieMedie);
  consumMediu += a * (e.pConsum - consumMediu);
  float productie = productieMedie;

  // daca se anunta vreme fara soare, pastram bateria de mai devreme
  float pragEconomie = 30;
  if (casa.vreme.valida && casa.vreme.soare < 25) pragEconomie = 45;

  ModEnergie nou = ENERGIE_NORMAL;
  if (e.soc < 12) nou = ENERGIE_CRITIC;
  else if (e.soc < pragEconomie && productie < consumMediu) nou = ENERGIE_ECONOMIE;
  else if (e.soc > 80 && productie > consumMediu) nou = ENERGIE_SURPLUS;
  // ca sa nu sarim intruna intre moduri: iesim din economie abia cand bateria
  // si-a revenit putin, iar din surplus abia cand chiar nu mai prisoseste
  if (e.mod == ENERGIE_ECONOMIE && nou == ENERGIE_NORMAL && e.soc < pragEconomie + 5) nou = ENERGIE_ECONOMIE;
  if (e.mod == ENERGIE_SURPLUS && nou == ENERGIE_NORMAL && e.soc > 75 && productie > consumMediu * 0.7) nou = ENERGIE_SURPLUS;

  if (nou != e.mod) {
    e.mod = nou;
    if (casa.mod == SMART) trimiteEveniment(EV_ENERGIE, nou, (uint8_t)e.soc);
  }
}

// Priza televizorului. In standby, un televizor consuma cativa wati zi si noapte
// (consum "fantoma"). SMART: daca nu e nimeni in sufragerie si televizorul doar
// sta in standby de 15 minute, taiem priza. Cand intra cineva, o pornim la loc.
void decidePriza() {
  Priza &p = casa.priza;
  // e cineva in sufragerie? (fara PIR: e cineva acasa)
  bool cineva = areMiscare() ? casa.camere[CAMERA_TV].prezenta : cinevaAcasa();

  if (!inScenariu()) {
    if (!isnan(p.putere)) p.tvPornit = p.pornita && p.putere > PRAG_TV_W;
    else p.tvPornit = p.pornita && cineva && oraZilei() >= 19 && oraZilei() < 23;  // ghicim
  }

  if (p.manual >= 0) { p.pornita = p.manual; return; }
  if (casa.mod == CLASIC) { p.pornita = true; return; }

  cineva = cineva && casa.alarma.stare != ALARMA_PLECAT;
  if (p.tvPornit || cineva) {
    if (!p.pornita) trimiteEveniment(EV_PRIZA_PORNITA, 0, 0);
    p.pornita = true;
    p.standbyDin = acum();
  } else if (p.pornita && acum() - p.standbyDin >= 900) {
    p.pornita = false;
    trimiteEveniment(EV_PRIZA_OPRITA, min(255UL, (acum() - p.standbyDin) / 60), 0);
  }
}

void reseteazaContoare() {
  Energie &e = casa.energie;
  e.whLumini = e.whIncalzire = e.whPompa = e.whPriza = 0;
  e.whClasic = 0;
  e.mwhMasurat = 0;
  e.litri = e.litriClasic = 0;
}

// Adunam energia folosita (la scara unei case adevarate) si, in paralel,
// cat ar fi folosit o casa clasica in exact aceleasi conditii.
void contorizeaza(uint32_t dt, uint32_t dtRealMs) {
  static uint16_t ziContoare = 0xFFFF;
  if (!inScenariu() && ziua() != ziContoare) {   // la miezul noptii o luam de la capat
    reseteazaContoare();
    ziContoare = ziua();
  }

  Energie &e = casa.energie;
  float ore = dt / 3600.0;
  uint16_t procenteBec = 0;       // suma luminozitatilor, 100 = un bec aprins complet
  uint8_t beciClasic = 0;
  for (uint8_t i = 0; i < NR_CAMERE; i++) {
    procenteBec += casa.camere[i].luminaTinta;
    beciClasic += casa.camere[i].aprinsaClasic;
  }
  e.whLumini += P_BEC * procenteBec / 100.0 * ore;
  if (casa.incalzire.pornita) e.whIncalzire += P_INCALZIRE * ore;
  float tv = casa.priza.tvPornit ? P_TV : P_STANDBY;
  if (casa.priza.pornita) e.whPriza += tv * ore;

  // casa geamana clasica: confort non-stop, luminile uitate, priza mereu pornita
  float clasicIncalzire = pasGeaman(dt) ? P_INCALZIRE : 0;
  e.whClasic += (clasicIncalzire + beciClasic * P_BEC + tv) * ore;

  // ce masuram cu adevarat pe macheta: W * ms / 3600 = mWh
  if (casa.senzori & SENZ_CONSUM) e.mwhMasurat += e.pMasurat * dtRealMs / 3600.0;
}
