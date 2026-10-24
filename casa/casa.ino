/*
  NEPHA - casa care se gandeste singura cum sa economiseasca energie
  -------------------------------------------------------------------
  Programul pentru Arduino Uno = "creierul" casei.

  Arduino citeste senzorii din macheta, ia toate deciziile (lumini,
  incalzire, irigatii, priza TV, alarma, energie) si comanda iesirile.
  Panoul cu ecran (ESP32) doar ne arata ce se intampla si ii trimite
  lui Arduino comenzile noastre (si ora + prognoza meteo de pe internet).

  Ce gasiti in fiecare tab:
    alarma    - armare / dezarmare cu PIN, intarzieri, sirena
    ceas      - ora casei (de la panou, sau accelerata in ziua de test)
    energie   - baterie, solar + vant, moduri de economie, priza TV, contoare
    gradina   - cand udam si cand nu
    lumini    - luminile din camere
    memorie   - ce pastram in EEPROM (setari, coduri senzori, rutina)
    panou     - mesajele schimbate cu ESP32 (si comenzile din Serial Monitor)
    rutina    - "inteligenta": invata la ce ore e lume prin casa
    scenariu  - ziua de test: aceeasi zi in modul clasic si in modul smart
    senzori   - citirea senzorilor adevarati
    termostat - incalzirea si ferestrele deschise
    usi       - senzorii wireless 433 MHz de pe usi si geamuri

  Biblioteci (Library Manager): rc-switch, OneWire
*/

#include "config.h"
#include "stare.h"
#include <Wire.h>
#include <EEPROM.h>
#include <RCSwitch.h>
#include <OneWire.h>
#if !defined(FARA_PANOU)
#include "legatura.h"
#endif

Casa casa;
Setari setari;

uint32_t ultimaDecizie = 0;    // millis() la ultima decizie
uint32_t secundaDecizie = 0;   // ceasul casei la ultima decizie

void setup() {
  Serial.begin(115200);
  Serial.println(F("\nNEPHA - creierul casei porneste..."));

  memorieIncarca();
  casa.mod = (ModCasa)setari.mod;

  camereLaZero(NAN);
  for (uint8_t i = 0; i < NR_CAMERE; i++) pinMode(PIN_LUMINA[i], OUTPUT);
  casa.tempAfara = NAN;
  casa.energie.vBaterie = NAN;
  casa.energie.soc = 60;
  casa.incalzire.rata = 3.0;      // pana invatam ceva mai bun
  casa.incalzire.tinta = T_CONFORT;
  casa.priza.pornita = true;
  casa.priza.manual = -1;
  casa.priza.putere = NAN;
  casa.gradina.umiditate = 50;
  casa.gradina.ziuaProgramator = 0xFFFF;
  casa.gradina.ziuaAnuntPloaie = 0xFFFF;
  casa.alarma.stare = ALARMA_OPRITA;
  casa.vreme.oraPloaie = 255;

  pinMode(PIN_INCALZIRE, OUTPUT);
  pinMode(PIN_POMPA, OUTPUT);
  pinMode(PIN_SIRENA, OUTPUT);
  pinMode(PIN_PRIZA, OUTPUT);
  digitalWrite(PIN_PRIZA, RELEU_PORNIT);

  panouInit();          // intai firul I2C (il folosesc si modulele INA219)
  senzoriInit();
  usiInit();

  secundaDecizie = acum();
  casa.priza.standbyDin = acum();
  Serial.println(F("Gata. Comenzi: AJUTOR"));
  trimiteEveniment(EV_PORNIT, casa.mod, 0);
}

void loop() {
  ceasPas();
  usiVerifica();        // codurile radio vin oricand, le citim des
  panouPas();           // comenzi de la panou + trimitem starea o data pe secunda
  if (!inScenariu()) senzoriCiteste();
  senzoriEnergie();

  uint32_t m = millis();
  if (m - ultimaDecizie >= 1000) {
    // o data pe secunda (reala) luam toate deciziile.
    // dt = cate secunde au trecut pe ceasul casei (in ziua de test pot fi minute)
    uint32_t dtReal = m - ultimaDecizie;
    ultimaDecizie = m;
    uint32_t dt = acum() - secundaDecizie;
    secundaDecizie = acum();

    if (inScenariu()) scenariuPas(dt);
    decide(dt);
    contorizeaza(dt, dtReal);
  }

  alarmaPas();
  aplicaIesiri();
}

// toate camerele "goale": fara miscare, lumini stinse, geamuri inchise
void camereLaZero(float temp) {
  memset(casa.camere, 0, sizeof(casa.camere));
  for (uint8_t i = 0; i < NR_CAMERE; i++) {
    casa.camere[i].temp = temp;
    casa.camere[i].medieLenta = NAN;
    casa.camere[i].manual = -1;
  }
}

// Ordinea conteaza: intai aflam ce se intampla (rutina, ferestre, energie),
// apoi decidem ce aprindem / pornim.
void decide(uint32_t dt) {
  if (dt > 900) dt = 900;   // daca ceasul a sarit (ora noua de la panou), nu "sarim" si cu modelele
  modelTermic(dt);
  rutinaPas(dt);
  detecteazaFerestre(dt);
  energieVirtuala(dt);
  decideEnergie(dt);
  decideIncalzire();
  decideLumini();
  decidePriza();
  decideGradina();
}

// Trimitem deciziile catre pini. Luminile se aprind si se sting lin.
void aplicaIesiri() {
  static uint32_t ultimPas = 0;
  if (millis() - ultimPas < 15) return;
  ultimPas = millis();

  for (uint8_t i = 0; i < NR_CAMERE; i++) {
    Camera &c = casa.camere[i];
    if (c.lumina < c.luminaTinta) c.lumina = min(c.luminaTinta, (uint8_t)(c.lumina + 2));
    else if (c.lumina > c.luminaTinta) c.lumina = max(c.luminaTinta, (uint8_t)(c.lumina - 2));
    // ochiul vede luminozitatea "in curba", asa ca ridicam la patrat
    uint16_t pwm = (uint16_t)c.lumina * c.lumina * 255UL / 10000UL;
    analogWrite(PIN_LUMINA[i], pwm);
  }

  digitalWrite(PIN_INCALZIRE, casa.incalzire.pornita ? HIGH : LOW);

  Gradina &g = casa.gradina;
  if (g.pompa && (int32_t)(millis() - g.pompaPanaLa) >= 0) g.pompa = false;
  digitalWrite(PIN_POMPA, g.pompa ? HIGH : LOW);

  digitalWrite(PIN_PRIZA, casa.priza.pornita ? RELEU_PORNIT : !RELEU_PORNIT);
  digitalWrite(PIN_SIRENA, sirenaSuna() ? HIGH : LOW);
}
