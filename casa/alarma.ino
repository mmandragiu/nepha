// alarma
//
// Senzorii wireless de pe usi si geamuri + senzorii de miscare din camere.
//   ACASA  (pe noapte) - suna doar daca se deschide o usa sau un geam
//   PLECAT             - suna si la miscare in casa
// Usa de la intrare are intarziere: cand intram, avem 20 s sa tastam PIN-ul.
//
// Legatura cu energia: cand armam PLECAT, casa stie sigur ca nu e nimeni,
// asa ca stinge luminile, lasa temperatura sa scada si taie priza TV-ului.

void alarmaSchimba(StareAlarma s, uint8_t senzor) {
  casa.alarma.stare = s;
  casa.alarma.din = millis();
  trimiteEveniment(EV_ALARMA, s, senzor);
}

void alarmaArmeaza(StareAlarma s) {
  Alarma &a = casa.alarma;
  if (a.stare != ALARMA_OPRITA) return;
  a.armare = s;
  a.sugerat = false;
  if (s == ALARMA_PLECAT && !inScenariu()) alarmaSchimba(ALARMA_IESIRE, 255);   // timp sa iesim
  else alarmaSchimba(s, 255);
}

void alarmaDezarmeaza(uint16_t pin) {
  Alarma &a = casa.alarma;
  if (a.stare == ALARMA_OPRITA) return;
  if ((int32_t)(millis() - a.blocataPanaLa) < 0) {     // prea multe PIN-uri gresite, asteptam
    trimiteEveniment(EV_PIN_GRESIT, 0, 0);
    return;
  }
  if (pin != setari.pin) {
    a.incercari++;
    uint8_t ramase = 3 - a.incercari;
    if (a.incercari >= 3) {
      a.incercari = 0;
      a.blocataPanaLa = millis() + 60000UL;
    }
    trimiteEveniment(EV_PIN_GRESIT, ramase, 0);
    return;
  }
  a.incercari = 0;
  alarmaSchimba(ALARMA_OPRITA, 255);
}

// s-a deschis o usa sau un geam (apelat din usi)
void alarmaSenzor(uint8_t slot) {
  StareAlarma s = casa.alarma.stare;
  if (s != ALARMA_ACASA && s != ALARMA_PLECAT) return;
  if (TIP_SENZOR[slot] == USA_INTRARE) alarmaSchimba(ALARMA_INTRARE, slot);
  else alarmaSchimba(ALARMA_SUNA, slot);
}

// miscare intr-o camera (apelat din senzori). 100 + camera = senzorul de miscare
void alarmaMiscare(uint8_t camera) {
  if (casa.alarma.stare == ALARMA_PLECAT) alarmaSchimba(ALARMA_INTRARE, 100 + camera);
}

void alarmaPas() {
  Alarma &a = casa.alarma;
  uint32_t secunde = (millis() - a.din) / 1000;
  switch (a.stare) {
    case ALARMA_IESIRE:
      if (secunde >= INTARZIERE_IESIRE_S) alarmaSchimba(a.armare, 255);
      break;
    case ALARMA_INTRARE:
      if (secunde >= INTARZIERE_INTRARE_S) alarmaSchimba(ALARMA_SUNA, 255);
      break;
    case ALARMA_SUNA:
      if (secunde >= DURATA_SIRENA_S) alarmaSchimba(a.armare, 255);   // tacem si ramanem armati
      break;
    default:
      break;
  }

  // Sugestie: casa e goala de o jumatate de ora si dupa rutina nu vine nimeni
  // in urmatoarele doua ore -> intrebam pe panou daca vrem sa armam.
  static uint32_t ultimaVerificare = 0;
  if (millis() - ultimaVerificare < 5000) return;
  ultimaVerificare = millis();
  if (a.stare != ALARMA_OPRITA || casa.mod != SMART || inScenariu()) return;
  if (eOraDeSomn(oraZilei())) return;     // noaptea dormim, nu e casa goala
  if (millis() < 1800000UL) return;       // abia am pornit: n-am vazut inca jumatate de ora fara nimeni
  if (cinevaAcasa()) {
    a.sugerat = false;
    return;
  }
  if (!a.sugerat && minutePanaVineCineva(120) < 0) {
    a.sugerat = true;
    trimiteEveniment(EV_SUGESTIE_ARMARE, 0, 0);
  }
}

// cum suna buzzer-ul in fiecare stare
bool sirenaSuna() {
  uint32_t ms = millis() - casa.alarma.din;
  switch (casa.alarma.stare) {
    case ALARMA_SUNA:    return (ms / 300) % 2 == 0;
    case ALARMA_INTRARE: return ms % 1000 < 80;     // bip scurt in fiecare secunda
    case ALARMA_IESIRE:  return ms % 2000 < 50;
    default:             return false;
  }
}
