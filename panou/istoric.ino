// istoric - tinem minte cum a mers casa, pentru grafice (ecran si pagina web)
//
// Un punct la fiecare 5 minute "de casa". In ziua de test ceasul casei merge
// accelerat, deci si punctele vin mai repede - si graficul arata toata ziua de test.

PunctIstoric istoric[NR_ISTORIC];
uint16_t nrIstoric = 0;
uint16_t ultimulMinutIstoric = 0xFFFF;

static int16_t zeci(float x) {
  return isnan(x) ? INT16_MIN : (int16_t)lroundf(x * 10);
}

static float medieCasa() {
  float suma = 0;
  uint8_t n = 0;
  for (uint8_t i = 0; i < NR_CAMERE; i++)
    if (!isnan(casa.camere[i].temp)) {
      suma += casa.camere[i].temp;
      n++;
    }
  return n ? suma / n : NAN;
}

void istoricPas() {
  if (!casa.primit) return;
  uint16_t cinci = casa.minut / 5;
  if (cinci == ultimulMinutIstoric) return;
  ultimulMinutIstoric = cinci;

  if (nrIstoric == NR_ISTORIC) {      // plin: aruncam cel mai vechi punct
    memmove(&istoric[0], &istoric[1], sizeof(PunctIstoric) * (NR_ISTORIC - 1));
    nrIstoric--;
  }
  PunctIstoric &p = istoric[nrIstoric++];
  p.minut = casa.minut;
  p.tCasa = zeci(medieCasa());
  p.tAfara = zeci(casa.tempAfara);
  p.baterie = casa.baterie;
  p.solar = isnan(casa.solar) ? 0 : casa.solar * 1000;
  p.vant = isnan(casa.vant) ? 0 : casa.vant * 1000;
  p.consum = isnan(casa.consum) ? 0 : casa.consum * 1000;
}
