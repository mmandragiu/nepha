// ceas - ora casei
//
// Arduino nu are ceas cu baterie, asa ca ora o primeste de la panou
// (ESP32 o ia de pe internet). Pana atunci pornim de la 07:00.
// In ziua de test ceasul merge accelerat: o ora trece in cateva secunde.

uint32_t ceasSecunde = 7UL * 3600;   // secunde de la pornire (ziua 0, ora 07:00)
uint16_t ceasViteza = 1;             // 1 = timp real
uint32_t ceasUltimMillis = 0;
uint16_t ceasRestMs = 0;
bool ceasSincronizat = false;

void ceasPas() {
  uint32_t m = millis();
  uint32_t ms = (m - ceasUltimMillis) * ceasViteza + ceasRestMs;
  ceasUltimMillis = m;
  ceasSecunde += ms / 1000;
  ceasRestMs = ms % 1000;
}

// folosit de testul rapid, unde nu asteptam deloc
void ceasAvanseaza(uint32_t secunde) {
  ceasSecunde += secunde;
}

// ora venita de la panou (minutul din zi). Pastram numaratoarea zilelor.
void ceasSeteaza(uint16_t minut) {
  uint32_t zi = ceasSecunde / 86400UL;
  uint32_t nou = zi * 86400UL + minut * 60UL;
  // daca ora noua e "in urma" cu mult, inseamna ca a trecut miezul noptii
  if (ceasSincronizat && nou + 43200UL < ceasSecunde) nou += 86400UL;
  // o corectie de cateva secunde e normala; un salt mare strica ora pe care o urmarea rutina
  uint32_t diferenta = nou > ceasSecunde ? nou - ceasSecunde : ceasSecunde - nou;
  if (diferenta > 120) {
    rutinaOraNoua();
    casa.priza.standbyDin = nou;     // altfel ar parea ca TV-ul a stat in standby cat a sarit ceasul
  }
  ceasSecunde = nou;
  secundaDecizie = nou;
  ceasRestMs = 0;
  ceasSincronizat = true;
  casa.senzori |= SENZ_CEAS;
}

uint32_t acum() { return ceasSecunde; }
uint8_t oraZilei() { return (ceasSecunde / 3600UL) % 24; }
uint16_t minutulZilei() { return (ceasSecunde / 60UL) % 1440; }
uint16_t ziua() { return ceasSecunde / 86400UL; }

bool eOraDeSomn(uint8_t ora) {
  return ora >= ORA_CULCARE || ora < ORA_TREZIRE;
}
