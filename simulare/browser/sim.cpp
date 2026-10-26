// sim.cpp - casa virtuala: programul lui Arduino (casa/) si al panoului (panou/),
// neschimbate, compilate impreuna pentru browser (Emscripten).
//
// Ca pe macheta: Arduino si panoul vorbesc pe firul I2C cu aceleasi mesaje (S, E, ORA...).
// Pagina (shell.html) arata ecranul panoului si casa, si are butoane in loc de senzori.
// Se construieste cu: node simulare/browser/construieste.mjs

#include "Arduino.h"
#include <vector>
#include <emscripten.h>

uint32_t simMillis = 31000;     // senzorii PIR sunt ignorati in primele 30 s dupa pornire
int simIntrari[32], simAnalog[32], simIesiri[32];
TwoWire Wire;
EEPROMClass EEPROM;
std::deque<CodRadio> RCSwitch::coada;

// ---------------------------------------------------------------- Arduino Uno
namespace uno {
SimSerial Serial;
#include "../../casa/casa.ino"
#include "../../casa/alarma.ino"
#include "../../casa/ceas.ino"
#include "../../casa/energie.ino"
#include "../../casa/gradina.ino"
#include "../../casa/lumini.ino"
#include "../../casa/memorie.ino"
#include "../../casa/panou.ino"
#include "../../casa/rutina.ino"
#include "../../casa/scenariu.ino"
#include "../../casa/senzori.ino"
#include "../../casa/termostat.ino"
#include "../../casa/usi.ino"
}

// ---------------------------------------------------------------- panoul ESP32
namespace panou {
SimSerial Serial;
int pini[40];
inline void pinMode(int, int) {}
inline void digitalWrite(int p, int v) {
  if (p >= 0 && p < 40) pini[p] = v;
}
void tone(int, unsigned frecventa, unsigned long durata = 0) {
  EM_ASM({ if (window.simTon) window.simTon($0, $1); }, frecventa, durata);
}
void noTone(int) {
  EM_ASM({ if (window.simTacere) window.simTacere(); });
}

#include "construit/panou/config.h"
#include "construit/panou/stare.h"
#include "construit/panou/ui.h"

CasaP casa;
RutinaP rutina;
RezultatTest rezultate[2];
EvenimentP jurnal[NR_JURNAL];
uint8_t nrJurnal = 0;
VremeP vreme;
float factorPanou = 0.8, latitudine = LATITUDINE, longitudine = LONGITUDINE;
bool areOra = true;

// in simulator "internetul" e mereu acolo
bool online() { return true; }
bool modAP() { return false; }
String adresaIP() { return "simulator"; }
String numeRetea() { return "casa virtuala"; }
void invataPanou() {}
// fara broker si fara imperechere in simulator
String idServer = "nepha-simulator";
void incepeImperechere() {}
uint32_t codDeAfisat() { return 0; }
uint8_t secundeImperechere() { return 0; }
void mqttEveniment(const EvenimentP &) {}
bool mqttConectat() { return false; }

// soarele din urmatoarele ore, dupa ora din zi (prognoza simulata)
float productieEstimata(uint8_t ore) {
  struct tm t;
  getLocalTime(&t, 0);
  float wh = 0;
  for (uint8_t i = 0; i < ore; i++) {
    float h = (t.tm_hour + i) % 24;
    float soare = std::max(0.0, std::sin((h - 7) / 11.0 * PI));
    wh += PM_SOLAR_MAX * soare * vreme.soare / 100.0 * factorPanou;
  }
  return wh;
}

#include "construit/panou/texte.ino"
#include "construit/panou/legatura.ino"
#include "construit/panou/istoric.ino"
#include "construit/panou/sunet.ino"
#include "construit/panou/ui.ino"
#include "construit/panou/ui_acasa.ino"
#include "construit/panou/ui_alarma.ino"
#include "construit/panou/ui_camere.ino"
#include "construit/panou/ui_energie.ino"
#include "construit/panou/ui_gradina.ino"
#include "construit/panou/ui_jurnal.ino"
#include "construit/panou/ui_rutina.ino"
#include "construit/panou/ui_setari.ino"
}

// ---------------------------------------------------------------- legatura cu pagina

static uint32_t miscarePanaLa[NR_CAMERE];
static bool usaDeschisa[NR_SENZORI_RF];
static uint32_t ultimulCodRadio = 0;
static std::string serialNou, fir;
static double inceput = -1;

// rutina unei familii obisnuite (aceeasi ca in ziua de test), ca pagina sa nu inceapa "goala":
// probabilitatea (%) pe ore, pentru Sufragerie, Dormitor, Bucatarie
static const uint8_t RUTINA_INVATATA[3][24] = {
  {10, 11, 11, 11, 11, 11, 11, 88, 66, 11, 11, 11, 11, 11, 11, 11, 11, 77, 88, 88, 88, 88, 88, 11},
  {90, 88, 88, 88, 88, 88, 88, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 77, 88},
  {10, 11, 11, 11, 11, 11, 88, 88, 22, 11, 11, 11, 11, 11, 11, 11, 11, 11, 77, 88, 11, 11, 11, 11}};

static int luminaBruta(int procent) { return LDR_INTUNERIC + (LDR_SOARE - LDR_INTUNERIC) * procent / 100; }
static int solBrut(int procent) { return SOL_USCAT + (SOL_UD - SOL_USCAT) * procent / 100; }

// ce a trecut pe firul I2C, pentru pagina (placile isi dau mesajele singure, prin Wire)
static void muta() {
  if (!Wire.deLaArduino.empty()) fir += "\x02" + Wire.deLaArduino;    // \x02 = de la Arduino spre panou
  Wire.deLaArduino.clear();
  if (!Wire.deLaPanou.empty()) fir += "\x01" + Wire.deLaPanou;        // \x01 = de la panou spre Arduino
  Wire.deLaPanou.clear();
  serialNou += uno::Serial.iesire;
  uno::Serial.iesire.clear();
}

// Arduino asteapta sa-i ia panoul ce are de spus: lasam timpul sa curga, iar panoul intreaba
void yield() {
  simMillis++;
  panou::legaturaPas();
}

extern "C" {

EMSCRIPTEN_KEEPALIVE void sim_porneste() {
  struct tm t;
  getLocalTime(&t, 0);
  bool zi = t.tm_hour >= 8 && t.tm_hour < 18;
  simAnalog[PIN_LDR] = luminaBruta(zi ? 70 : 4);
  simAnalog[PIN_SOL] = solBrut(45);
  simIesiri[PIN_PRIZA] = RELEU_PORNIT;

  uno::setup();
  // senzorii de pe usi si geamuri: ii "invatam" direct (pe macheta se face din panou)
  for (uint8_t i = 0; i < NR_SENZORI_RF; i++) {
    uno::coduriRF[i] = 1001 + i;
    uno::salveazaCodRF(i, 1001 + i);
  }
  for (uint8_t h = 0; h < 24; h++) {
    uint8_t zile = h == 0 ? 8 : 7;
    uno::rutinaZile[h] = zile;
    for (uint8_t i = 0; i < NR_CAMERE; i++)
      uno::rutinaOcupat[i][h] = (uint8_t)std::max(0.0, std::round(RUTINA_INVATATA[i][h] * (zile + 2) / 100.0 - 1));
    uno::salveazaOraRutina(h);
  }

  panou::vreme.valida = true;
  panou::vreme.temp = 9;
  panou::vreme.ploaie = 10;
  panou::vreme.oraPloaie = 255;
  panou::vreme.soare = zi ? 60 : 0;
  panou::legaturaInit();
  panou::uiInit();
}

// pagina ne cheama la fiecare cadru, cu timpul in milisecunde
EMSCRIPTEN_KEEPALIVE void sim_pas(double acum) {
  if (inceput < 0) inceput = acum;
  uint32_t tinta = 31000 + (uint32_t)(acum - inceput);
  int pasi = 0;
  while (simMillis < tinta && pasi < 3000) {
    simMillis += 5;
    pasi++;
    for (uint8_t i = 0; i < NR_CAMERE; i++) simIntrari[uno::PIN_PIR[i]] = simMillis < miscarePanaLa[i] ? HIGH : LOW;
    if (simMillis - ultimulCodRadio >= 300) {      // un senzor de usa transmite cat timp e deschis
      ultimulCodRadio = simMillis;
      for (uint8_t i = 0; i < NR_SENZORI_RF; i++)
        if (usaDeschisa[i]) RCSwitch::coada.push_back({1001u + i, 24});
    }
    uno::loop();
    muta();
  }
  if (simMillis < tinta) simMillis = tinta;    // pagina a stat in fundal: sarim peste
  panou::legaturaPas();
  muta();
  panou::istoricPas();
  panou::uiPas();
  panou::sunetPas();
  panou::ledPas();
}

EMSCRIPTEN_KEEPALIVE uint8_t *sim_ecran() { return (uint8_t *)panou::tft.px; }

EMSCRIPTEN_KEEPALIVE void sim_atinge(int x, int y) {
  panou::tft.atingereX = x;
  panou::tft.atingereY = y;
}

EMSCRIPTEN_KEEPALIVE void sim_miscare(int camera) {
  if (camera >= 0 && camera < NR_CAMERE) miscarePanaLa[camera] = simMillis + 4000;
}

EMSCRIPTEN_KEEPALIVE void sim_usa(int senzor, int deschis) {
  if (senzor >= 0 && senzor < NR_SENZORI_RF) usaDeschisa[senzor] = deschis;
}

EMSCRIPTEN_KEEPALIVE void sim_lumina(int procent) { simAnalog[PIN_LDR] = luminaBruta(procent); }
EMSCRIPTEN_KEEPALIVE void sim_sol(int procent) { simAnalog[PIN_SOL] = solBrut(procent); }

EMSCRIPTEN_KEEPALIVE void sim_vreme(float temp, int ploaie, int oraPloaie, int soare) {
  panou::vreme.temp = temp;
  panou::vreme.ploaie = ploaie;
  panou::vreme.oraPloaie = oraPloaie;
  panou::vreme.soare = soare;
  panou::ultimaVremeTrimisa = 0;     // panoul i-o trimite imediat lui Arduino
}

// o comanda scrisa in "Serial Monitor"
EMSCRIPTEN_KEEPALIVE void sim_comanda(const char *c) {
  uno::Serial.primeste(c);
  uno::Serial.primeste("\n");
}

// ce a scris Arduino pe Serial de la ultima intrebare
EMSCRIPTEN_KEEPALIVE const char *sim_serial() {
  static std::string dat;
  dat.swap(serialNou);
  serialNou.clear();
  return dat.c_str();
}

// mesajele de pe fir de la ultima intrebare
EMSCRIPTEN_KEEPALIVE const char *sim_fir() {
  static std::string dat;
  dat.swap(fir);
  fir.clear();
  return dat.c_str();
}

// starea machetei pentru desenul casei
EMSCRIPTEN_KEEPALIVE int sim_iesire(int i) {
  uno::Casa &c = uno::casa;
  switch (i) {
    case 0: case 1: case 2: return simIesiri[uno::PIN_LUMINA[i]];
    case 3: return simIesiri[PIN_INCALZIRE];
    case 4: return simIesiri[PIN_POMPA];
    case 5: return simIesiri[PIN_PRIZA] == RELEU_PORNIT;
    case 6: return simIesiri[PIN_SIRENA];
    case 7: return c.alarma.stare;
    case 8: case 9: case 10: return c.camere[i - 8].prezenta;
    case 11: case 12: case 13: return isnan(c.camere[i - 11].temp) ? -1000 : (int)std::lround(c.camere[i - 11].temp * 10);
    case 14: return isnan(c.tempAfara) ? (int)std::lround(c.vreme.tempAfara * 10) : (int)std::lround(c.tempAfara * 10);
    case 15: return (int)c.energie.soc;
    case 16: return c.gradina.umiditate;
    case 17: return c.usiDeschise;
    case 18: return c.luminaNaturala;
    case 19: return c.mod;
    case 20: return uno::stareScenariu();
    case 21: return c.priza.tvPornit;
    case 22: return c.camere[0].fereastra | c.camere[1].fereastra << 1 | c.camere[2].fereastra << 2;
    case 23: return uno::minutulZilei();
    case 24: return panou::luminaEcran;
  }
  return 0;
}
}
