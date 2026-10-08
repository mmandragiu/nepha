// gradina - cand udam
//
// CLASIC: programator - in fiecare zi la 12:00, indiferent de vreme.
// SMART:  udam doar cand pamantul chiar e uscat si nu vine ploaia.
//         Preferam dimineata si seara (se evapora mai putin apa), iar daca
//         la pranz avem surplus de soare, il folosim pentru pompa.

enum MotivUdare : uint8_t { MU_NIMIC, MU_SURPLUS, MU_ORA_BUNA, MU_CRITIC, MU_PROGRAMATOR, MU_MANUAL };

void pornestePompa(uint8_t motiv) {
  Gradina &g = casa.gradina;
  g.ultimaUdare = acum();
  g.motiv = motiv;
  casa.energie.whPompa += KWH_UDARE * 1000;
  casa.energie.litri += LITRI_UDARE;
  if (stareScenariu() != 2) {          // in testul rapid nu pornim pompa de-adevaratelea
    g.pompa = true;
    g.pompaPanaLa = millis() + DURATA_POMPA_S * 1000UL;
  }
  trimiteEveniment(EV_UDARE, motiv, g.umiditate);
  if (inScenariu()) g.umiditate = min(100, g.umiditate + 30);   // pamantul din test e virtual
}

void decideGradina() {
  Gradina &g = casa.gradina;
  uint8_t ora = oraZilei();

  // programatorul clasic: il urmarim mereu, ca sa avem cu ce compara
  if (ora == 12 && g.ziuaProgramator != ziua()) {
    g.ziuaProgramator = ziua();
    casa.energie.whClasic += KWH_UDARE * 1000;
    casa.energie.litriClasic += LITRI_UDARE;
    if (casa.mod == CLASIC) pornestePompa(MU_PROGRAMATOR);
  }
  if (casa.mod == CLASIC || g.pompa) return;

  if (g.umiditate >= PRAG_UDARE) return;                     // pamantul e bine
  if (acum() - g.ultimaUdare < 4 * 3600UL) return;           // am udat de curand, lasam apa sa intre
  bool critic = g.umiditate < PRAG_CRITIC;

  // vine ploaia? lasam natura sa ude (scriem in jurnal o singura data pe zi)
  if (!critic && casa.vreme.valida && casa.vreme.ploaie >= 60) {
    if (g.ziuaAnuntPloaie != ziua()) {
      g.ziuaAnuntPloaie = ziua();
      trimiteEveniment(EV_UDARE_AMANATA, casa.vreme.ploaie, casa.vreme.oraPloaie);
    }
    return;
  }

  bool oraBuna = (ora >= 6 && ora < 10) || (ora >= 18 && ora < 21);
  ModEnergie en = casa.energie.mod;
  bool energieSlaba = en == ENERGIE_ECONOMIE || en == ENERGIE_CRITIC;

  if (critic) pornestePompa(MU_CRITIC);
  else if (en == ENERGIE_SURPLUS && ora >= 9 && ora < 18) pornestePompa(MU_SURPLUS);
  else if (oraBuna && !energieSlaba) pornestePompa(MU_ORA_BUNA);
}
