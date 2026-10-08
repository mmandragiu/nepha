// lumini - becurile din camere
//
// CLASIC: aprindem cand intram si e intuneric, dar nu ne mai intoarcem sa
//         stingem - lumina ramane aprinsa pana pleaca toata lumea sau ne culcam.
// SMART:  lumina doar in camerele unde e cineva, si doar cat lipseste din
//         lumina naturala (seara devreme becul merge mai slab). Cat asteptam
//         dupa ultima miscare depinde de rutina: la orele cand de obicei stam
//         in camera, asteptam mai mult (poate citim si nu ne miscam).

#define PRAG_INTUNERIC 35    // sub acest nivel de lumina naturala e nevoie de bec
#define NIVEL_DORIT    85    // cat de luminata vrem camera

// cat timp tinem lumina aprinsa dupa ultima miscare, in secunde (2.5 .. 7.5 minute)
static uint32_t timpStingere(uint8_t camera) {
  uint8_t p = probabilitate(camera, oraZilei());
  return 150 + 3UL * p;
}

// e cineva acasa? (miscare in ultima jumatate de ora in vreo camera)
bool cinevaAcasa() {
  if (casa.alarma.stare == ALARMA_PLECAT) return false;
  for (uint8_t i = 0; i < NR_CAMERE; i++) {
    if (casa.camere[i].prezenta) return true;
    if (acum() - casa.camere[i].ultimaMiscare < 1800 && casa.camere[i].ultimaMiscare > 0) return true;
  }
  return false;
}

static uint8_t luminaSmart(uint8_t i) {
  Camera &c = casa.camere[i];
  if (casa.alarma.stare == ALARMA_PLECAT) return 0;
  if (casa.alarma.stare == ALARMA_SUNA) return 100;          // aprindem tot, sa se vada
  bool aprinsa = c.luminaTinta > 0;
  // histerezis: aprindem sub 35, stingem peste 45
  if (casa.luminaNaturala >= PRAG_INTUNERIC + 10) return 0;
  if (!aprinsa && casa.luminaNaturala >= PRAG_INTUNERIC) return 0;

  bool cineva = c.prezenta || (c.ultimaMiscare > 0 && acum() - c.ultimaMiscare < timpStingere(i));
  // noaptea, cand dormim, lumina se aprinde doar daca s-a miscat cineva acum (ne-am trezit) si slab
  bool noapte = eOraDeSomn(oraZilei());
  if (noapte) cineva = c.ultimaMiscare > 0 && acum() - c.ultimaMiscare < 120;
  if (!cineva) {
    if (aprinsa) trimiteEveniment(EV_LUMINA_STINSA, i, 0);
    return 0;
  }
  if (noapte) return 20;
  // completam doar ce lipseste din lumina de afara
  int nivel = NIVEL_DORIT - casa.luminaNaturala;
  if (nivel < 25) nivel = 25;
  if (casa.energie.mod == ENERGIE_ECONOMIE && nivel > 60) nivel = 60;
  if (casa.energie.mod == ENERGIE_CRITIC && nivel > 30) nivel = 30;
  return nivel;
}

static uint8_t luminaClasic(uint8_t i) {
  Camera &c = casa.camere[i];
  if (!cinevaAcasa() || eOraDeSomn(oraZilei())) c.aprinsaClasic = false;
  else if (c.prezenta && casa.luminaNaturala < PRAG_INTUNERIC) c.aprinsaClasic = true;
  return c.aprinsaClasic ? 100 : 0;
}

void decideLumini() {
  for (uint8_t i = 0; i < NR_CAMERE; i++) {
    Camera &c = casa.camere[i];
    // varianta clasica o calculam mereu, ca sa stim cat ar fi consumat o casa obisnuita
    uint8_t clasic = luminaClasic(i);
    if (c.manual >= 0 && acum() < c.manualPanaLa) {
      c.luminaTinta = c.manual;            // a apasat cineva pe panou - respectam o ora
      continue;
    }
    c.manual = -1;
    c.luminaTinta = casa.mod == SMART ? luminaSmart(i) : clasic;
  }
}
