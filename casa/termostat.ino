// termostat - incalzirea casei
//
// CLASIC: un termostat obisnuit, tine temperatura de confort zi si noapte.
// SMART:
//   - cand nu e nimeni acasa sau dormim, lasa temperatura sa scada
//   - porneste mai devreme daca stie din rutina ca vine cineva; cat de
//     devreme depinde de cat de repede se incalzeste casa (si asta o invata)
//   - opreste incalzirea cand e o fereastra deschisa (de la senzor sau
//     dedusa din faptul ca temperatura scade brusc)
//   - daca bateria e pe terminate, coboara un grad sau doua

enum MotivIncalzire : uint8_t {
  MI_CLASIC, MI_CONFORT, MI_PREINCALZIRE, MI_PLECATI, MI_NIMENI, MI_NOAPTE, MI_FEREASTRA
};

// constante pentru modelul termic (grade pe ora) - le folosim cand nu avem
// senzor intr-o camera si in ziua de test
#define PIERDERE_PERETI   0.08   // cat pierde casa prin pereti, pe grad de diferenta (casa veche, neizolata)
#define PIERDERE_GEAM     1.0    // cat pierde in plus cu geamul deschis
#define CASTIG_CALORIFER  4.0    // cat castiga cand merge incalzirea

float tempCasa() {
  float suma = 0;
  uint8_t n = 0;
  for (uint8_t i = 0; i < NR_CAMERE; i++) {
    if (!isnan(casa.camere[i].temp)) {
      suma += casa.camere[i].temp;
      n++;
    }
  }
  return n ? suma / n : NAN;
}

static float tempAfaraSauPrognoza() {
  if (!isnan(casa.tempAfara)) return casa.tempAfara;
  if (casa.vreme.valida) return casa.vreme.tempAfara;
  return 10.0;
}

// Pentru camerele fara senzor (si pentru toate, in ziua de test) calculam
// temperatura cu un model simplu: pierde caldura spre afara, castiga cand incalzim.
void modelTermic(uint32_t dt) {
  float ore = dt / 3600.0;
  float afara = tempAfaraSauPrognoza();
  for (uint8_t i = 0; i < NR_CAMERE; i++) {
    if (!inScenariu() && (camereCuSenzor & (1 << i))) continue;   // avem temperatura reala
    Camera &c = casa.camere[i];
    if (isnan(c.temp)) c.temp = 20.0;
    float pierdere = PIERDERE_PERETI * (c.temp - afara);
    if (c.geamDeschis) pierdere += PIERDERE_GEAM * (c.temp - afara);
    float castig = casa.incalzire.pornita ? CASTIG_CALORIFER : 0;
    c.temp += (castig - pierdere) * ore;
  }
}

// "Casa geamana": o copie virtuala a casei noastre, dar cu un termostat clasic
// (confort zi si noapte, nu vede geamurile deschise). O lasam sa "traiasca" in
// paralel, cu aceeasi vreme si aceleasi geamuri, ca sa stim cat ar fi consumat.
float tempGeaman = NAN;
bool caloriferGeaman = false;

// un pas din viata casei gemene; intoarce true daca acum i-ar merge caloriferul
bool pasGeaman(uint32_t dt) {
  float confort = setari.confort / 10.0;
  if (isnan(tempGeaman)) tempGeaman = isnan(tempCasa()) ? confort : tempCasa();
  if (tempGeaman < confort - 0.3) caloriferGeaman = true;
  else if (tempGeaman > confort + 0.2) caloriferGeaman = false;
  uint8_t geamuri = 0;
  for (uint8_t i = 0; i < NR_CAMERE; i++) geamuri += casa.camere[i].geamDeschis;
  float pierdere = (PIERDERE_PERETI + PIERDERE_GEAM * geamuri / NR_CAMERE) * (tempGeaman - tempAfaraSauPrognoza());
  tempGeaman += ((caloriferGeaman ? CASTIG_CALORIFER : 0) - pierdere) * dt / 3600.0;
  return caloriferGeaman;
}

static bool areSenzorGeam(uint8_t camera) {
  for (uint8_t s = 0; s < NR_SENZORI_RF; s++)
    if (TIP_SENZOR[s] == FEREASTRA && CAMERA_SENZOR[s] == camera) return true;
  return false;
}

// Fereastra deschisa iarna = aer rece = temperatura scade mult mai repede
// decat de obicei. Comparam temperatura de acum cu media ultimelor ~5 minute:
// daca e cu peste 0.5 grade sub medie, aproape sigur e un geam deschis.
void detecteazaFerestre(uint32_t dt) {
  for (uint8_t i = 0; i < NR_CAMERE; i++) {
    Camera &c = casa.camere[i];
    if (isnan(c.temp)) continue;
    if (isnan(c.medieLenta)) c.medieLenta = c.temp;
    float a = dt / (300.0 + dt);
    c.medieLenta += a * (c.temp - c.medieLenta);

    bool inainte = c.fereastra;
    bool dedusa = c.medieLenta - c.temp >= 0.5;
    if (dedusa) c.fereastraPanaLa = acum() + 15 * 60;   // pauza 15 minute
    bool dinSenzor = c.geamDeschis && areSenzorGeam(i);
    c.fereastra = dinSenzor || acum() < c.fereastraPanaLa;

    if (casa.mod != SMART) continue;   // casa clasica nu baga de seama
    if (c.fereastra && !inainte) trimiteEveniment(EV_FEREASTRA, i, dinSenzor ? 2 : 1);
    if (!c.fereastra && inainte) trimiteEveniment(EV_FEREASTRA_GATA, i, 0);
  }
}

static bool oFereastraDeschisa() {
  for (uint8_t i = 0; i < NR_CAMERE; i++)
    if (casa.camere[i].fereastra) return true;
  return false;
}

// Spre ce temperatura mergem in modul smart. Seteaza si motivul.
static float tintaSmart(uint8_t &motiv, int16_t &vine) {
  float confort = setari.confort / 10.0;
  float t = tempCasa();
  StareAlarma al = casa.alarma.stare;

  // 1. temperatura "redusa" de acum, daca n-ar veni nimeni
  float redusa;
  if (al == ALARMA_PLECAT || al == ALARMA_IESIRE) {
    motiv = MI_PLECATI;
    redusa = setari.plecat / 10.0;
  } else if (eOraDeSomn(oraZilei()) || al == ALARMA_ACASA) {
    motiv = MI_NOAPTE;
    redusa = setari.noapte / 10.0;
  } else if (cinevaAcasa()) {
    motiv = MI_CONFORT;
    return confort;
  } else {
    motiv = MI_NIMENI;
    redusa = setari.eco / 10.0;
  }

  // 2. dar vine cineva curand? Cat ii ia casei sa ajunga la confort, din
  //    rata invatata (+20 min rezerva), si ne uitam in rutina atat de departe.
  uint16_t avans = 20;
  if (t < confort) avans += (uint16_t)((confort - t) / casa.incalzire.rata * 60.0);
  if (avans > 180) avans = 180;
  vine = minutePanaVineCineva(avans);

  // odata ce am pornit preincalzirea, o tinem pana la ora estimata (+30 min).
  // Altfel, cum se incalzeste casa, "avansul" scade si am porni-opri intruna.
  Incalzire &inc = casa.incalzire;
  if (vine >= 0) inc.preincalzirePanaLa = max(inc.preincalzirePanaLa, acum() + (vine + 30) * 60UL);
  if (acum() < inc.preincalzirePanaLa) {
    motiv = MI_PREINCALZIRE;
    return confort;
  }
  return redusa;
}

void decideIncalzire() {
  Incalzire &inc = casa.incalzire;
  float t = tempCasa();
  if (isnan(t)) { inc.pornita = false; return; }

  uint8_t motiv = MI_CLASIC;
  int16_t vine = -1;
  float tinta = setari.confort / 10.0;
  if (casa.mod == SMART) {
    tinta = tintaSmart(motiv, vine);
    if (casa.energie.mod == ENERGIE_ECONOMIE) tinta -= 1.0;
    if (casa.energie.mod == ENERGIE_CRITIC) tinta -= 2.0;
    if (oFereastraDeschisa()) motiv = MI_FEREASTRA;
  }

  // anuntam in jurnal doar cand se schimba ceva
  if (motiv != inc.motiv) {
    if (motiv == MI_PREINCALZIRE && vine >= 0)
      trimiteEveniment(EV_PREINCALZIRE, vine, ((minutulZilei() + vine) / 60) % 24);
    else if (motiv == MI_PLECATI) trimiteEveniment(EV_INCALZIRE_REDUSA, 0, 0);
    else if (motiv == MI_NIMENI) trimiteEveniment(EV_INCALZIRE_REDUSA, 1, 0);
    else if (motiv == MI_NOAPTE) trimiteEveniment(EV_INCALZIRE_REDUSA, 2, 0);
    inc.motiv = motiv;
  }
  inc.tinta = tinta;

  bool inainte = inc.pornita;
  if (motiv == MI_FEREASTRA) inc.pornita = false;
  else if (t < tinta - 0.3) inc.pornita = true;     // histerezis, ca sa nu tot porneasca si opreasca
  else if (t > tinta + 0.2) inc.pornita = false;

  // invatam cat de repede se incalzeste casa: masuram la fiecare 15 minute de mers
  if (inc.pornita && !inainte) {
    inc.pornitaLa = acum();
    inc.tempLaPornire = t;
  }
  if (inc.pornita && acum() - inc.pornitaLa >= 900) {
    float ore = (acum() - inc.pornitaLa) / 3600.0;
    float rataNoua = (t - inc.tempLaPornire) / ore;
    if (rataNoua > 0.3 && rataNoua < 15) inc.rata = inc.rata * 0.7 + rataNoua * 0.3;
    inc.pornitaLa = acum();
    inc.tempLaPornire = t;
  }
}
