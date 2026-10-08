// texte - ce scriem pe ecran si pe pagina web pentru fiecare lucru facut de casa.
// Arduino trimite doar coduri (n-are loc de texte), noi le facem propozitii.
// Codurile sunt aceleasi ca in casa/stare.h (enum Eveniment).

String oraText(uint16_t minut) {
  char s[6];
  snprintf(s, sizeof(s), "%02d:%02d", (minut / 60) % 24, minut % 60);
  return s;
}

static String numeCamera(uint8_t i) {
  return i < NR_CAMERE ? NUME_CAMERE[i] : "o cameră";
}

static String numeSenzor(uint8_t i) {
  if (i < NR_SENZORI_RF) return NUME_SENZORI[i];
  if (i >= 100 && i < 100 + NR_CAMERE) return "Mișcare în " + numeCamera(i - 100);
  return "";
}

const char *textMotiv(uint8_t motiv) {
  switch (motiv) {
    case 0: return "termostat clasic";
    case 1: return "e cineva acasă";
    case 2: return "preîncălzire, vine cineva";
    case 3: return "ați plecat";
    case 4: return "nu e nimeni prin casă";
    case 5: return "temperatura de noapte";
    case 6: return "fereastră deschisă";
  }
  return "";
}

const char *textAlarma(uint8_t stare) {
  switch (stare) {
    case 0: return "Dezarmată";
    case 1: return "Se armează...";
    case 2: return "Armată (noapte)";
    case 3: return "Armată (plecat)";
    case 4: return "Introduceți PIN-ul!";
    case 5: return "ALARMĂ!";
  }
  return "";
}

const char *textEnergie(uint8_t mod) {
  switch (mod) {
    case 1: return "Surplus";
    case 2: return "Economie";
    case 3: return "Critic";
  }
  return "Normal";
}

String textEveniment(const EvenimentP &e) {
  uint8_t a = e.a, b = e.b;
  switch (e.cod) {
    case 1:
      return a ? "Casa a pornit. Modul Smart: decide singură." : "Casa a pornit, în modul clasic.";
    case 2:
      return a ? "Am trecut pe modul Smart: casa decide singură."
               : "Am trecut pe modul clasic (ca o casă obișnuită, pentru comparație).";
    case 3:
      return "Pornesc încălzirea mai devreme: de obicei e cineva acasă pe la " + String(b) +
             ":00 (peste ~" + String(a) + " min).";
    case 4:
      if (a == 0) return "Ați plecat: las casa să se răcească până vă întoarceți.";
      if (a == 1) return "Nu e nimeni prin casă: trec pe temperatura de economie.";
      return "Noapte bună: trec pe temperatura de noapte.";
    case 5:
      if (b == 2) return "Fereastra din " + numeCamera(a) + " e deschisă: opresc încălzirea până se închide.";
      return "În " + numeCamera(a) + " temperatura scade brusc, pare o fereastră deschisă. Opresc încălzirea 15 minute.";
    case 6:
      return "Fereastra din " + numeCamera(a) + " s-a închis, încălzirea merge din nou.";
    case 7:
      return "Am stins lumina în " + numeCamera(a) + ": nu mai era nimeni.";
    case 8:
      return "Am oprit priza televizorului: stătea în standby de " + String(a) + " min.";
    case 9:
      return "A intrat cineva în sufragerie: am pornit priza televizorului.";
    case 10:
      switch (a) {
        case 1: return "Ud grădina acum: bateria e plină și panoul încă produce (pământ " + String(b) + "%).";
        case 2: return "Ud grădina: pământul e uscat (" + String(b) + "%) și e o oră bună, se evaporă puțin.";
        case 3: return "Pământul e foarte uscat (" + String(b) + "%), ud acum.";
        case 4: return "Programatorul a pornit udarea, la 12 ca în fiecare zi.";
        default: return "Udare pornită de mână.";
      }
    case 11:
      return "Nu ud grădina: vine ploaia (" + String(a) + "% pe la ora " + String(b) + ").";
    case 12:
      switch (a) {
        case 1: return "Bateria e plină (" + String(b) + "%) și panoul produce: e momentul pentru ce poate aștepta.";
        case 2: return "Bateria scade (" + String(b) + "%) și nu se produce nimic: lumini mai slabe, un grad mai puțin.";
        case 3: return "Bateria e aproape goală (" + String(b) + "%): doar strictul necesar.";
        default: return "Energia e din nou normală (baterie " + String(b) + "%).";
      }
    case 13:
      switch (a) {
        case 0: return "Alarma a fost dezarmată. Bine ați venit!";
        case 1: return "Alarma se armează: aveți 30 de secunde să ieșiți.";
        case 2: return "Alarma e armată pentru noapte (uși și geamuri).";
        case 3: return "Alarma e armată. Casa știe că nu e nimeni și economisește.";
        case 4: return numeSenzor(b) + ": introduceți PIN-ul în 20 de secunde!";
        default: return "ALARMĂ! " + numeSenzor(b);
      }
    case 14:
      return numeSenzor(a) + (b ? " s-a deschis." : " s-a închis.");
    case 15:
      return "Casa pare goală și nu aștept pe nimeni curând. Armez alarma? Așa economisește și energie.";
    case 16:
      if (a) return "PIN greșit. Mai aveți " + String(a) + (a == 1 ? " încercare." : " încercări.");
      return "Prea multe încercări greșite: tastatura e blocată un minut.";
    case 17:
      if (b) return "Am învățat senzorul „" + numeSenzor(a) + "”.";
      return "N-am auzit niciun senzor în 20 de secunde. Mai încercați o dată.";
    case 18:
      if (a == 1) return String("Începe ziua de test, ca o casă ") + (b ? "smart." : "clasică.");
      if (a == 2) return String("Ziua de test s-a terminat (") + (b ? "smart)." : "clasic).");
      return "Ziua de test a fost oprită.";
  }
  return "Eveniment " + String(e.cod);
}
