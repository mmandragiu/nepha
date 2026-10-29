// stare.h - ce stie panoul despre casa (vine de la Arduino) + ce aduce el de pe internet
#pragma once
#include <Arduino.h>
#include "config.h"

struct CameraP {
  float temp;
  uint8_t lumina;      // 0..100
  bool prezenta;
  bool fereastra;
};

// ultima linie "S ..." primita de la Arduino, desfacuta pe bucati
struct CasaP {
  bool primit;          // am primit macar o stare de la Arduino
  uint32_t primitLa;    // millis()
  uint16_t minut;       // ora casei (minutul din zi)
  uint8_t mod;          // 0 clasic, 1 smart
  uint8_t test;         // 0 nu, 1 ziua de test pe macheta, 2 test rapid
  CameraP camere[NR_CAMERE];
  float tempAfara;
  uint8_t luminaNat;
  bool incalzire;
  float tinta;
  uint8_t motiv;        // de ce: 0 clasic, 1 confort, 2 preincalzire, 3 plecati, 4 nimeni, 5 noapte, 6 fereastra
  float rata;           // cate grade pe ora castiga casa cand incalzeste (invatat)
  uint8_t baterie;
  float solar, vant, consum;   // W pe macheta
  uint8_t modEnergie;   // 0 normal, 1 surplus, 2 economie, 3 critic
  uint8_t alarma;       // 0 oprita, 1 iesire, 2 acasa, 3 plecat, 4 intrare, 5 suna
  uint8_t usi;          // bitul i = senzorul i deschis
  uint8_t sol;
  bool pompa;
  uint8_t umiditateAer; // % de la DHT11 (0 = fara senzor)
  float confort;        // temperatura de confort setata pe Arduino
  bool priza;
  float whAzi, whClasic, mwh;
  uint8_t senzori;
};

// placa din gradina (ESP-NOW, vezi gradina.ino)
struct GradinaP {
  bool aici;            // o auzim (mesaj in ultimele 30 s)
  uint32_t la;          // millis() la ultimul mesaj
  bool ploua;
  uint8_t zona;         // ce zona se uda acum, 0 = niciuna
};

struct EvenimentP {
  uint16_t minut;
  uint8_t cod, a, b;
};

struct RezultatTest {
  bool valid;
  float lumini, incalzire, pompa, priza, total, mwh, litri;
};

struct VremeP {
  bool valida;
  uint32_t la;          // millis() cand am luat-o
  float temp, vant;
  uint8_t ploaie;       // % maxim in urmatoarele 12 ore
  uint8_t oraPloaie;    // ora cand e cea mai mare sansa (255 = nu ploua)
  uint8_t soare;        // 0..100 cat soare vine in urmatoarele 6 ore
  uint8_t nori;         // % acum
  float radiatie[12];   // W/m2, pe urmatoarele 12 ore
};

struct RutinaP {
  bool valida;
  uint8_t p[NR_CAMERE][24];   // probabilitatea (%) sa fie cineva in camera la ora aia
  uint8_t zile[24];
};

// un punct pe grafic, la fiecare 5 minute
struct PunctIstoric {
  uint16_t minut;
  int16_t tCasa, tAfara;      // x10
  uint8_t baterie;
  uint16_t solar, vant, consum;   // mW
};

#define NR_JURNAL 40
#define NR_ISTORIC 288        // 24 de ore x 12 puncte pe ora

extern CasaP casa;
extern GradinaP gradina;
extern VremeP vreme;
extern RutinaP rutina;
extern RezultatTest rezultate[2];
extern EvenimentP jurnal[NR_JURNAL];
extern uint8_t nrJurnal;
extern PunctIstoric istoric[NR_ISTORIC];
extern uint16_t nrIstoric;
extern float factorPanou;     // cat din "ce ar trebui sa produca" produce de fapt panoul (invatat)
extern float latitudine, longitudine;
extern bool areOra;           // avem ora de pe internet
extern uint32_t ultimaVremeTrimisa;

// legatura
void legaturaInit();
void legaturaPas();
void trimiteArduino(const char *comanda);

// retea
void reteaInit();
void reteaPas();
bool online();
String adresaIP();
String numeRetea();
bool modAP();
void salveazaWifi(const String &nume, const String &parola);

// vreme
void vremePas();
float productieEstimata(uint8_t ore);
void invataPanou();

// istoric
void istoricPas();

// gradina (placa cu senzorul de ploaie si releele)
void gradinaInit();
void gradinaPas();

// imperechere si broker (folosite si de ecran si de legatura cu Arduino)
extern String idServer;
void incepeImperechere();
uint32_t codDeAfisat();
uint8_t secundeImperechere();
void mqttEveniment(const EvenimentP &e);
bool mqttConectat();

// doar pe placa (simulatorul din browser nu are retea)
#if defined(ARDUINO_ARCH_ESP32)
#include <ArduinoJson.h>

// web (JSON-ul e acelasi pe Wi-Fi si prin broker)
void webInit();
void webPas();
void jsonStare(JsonObject doc);
void jsonJurnal(JsonArray a);
void jsonRutina(JsonObject doc);
void jsonEveniment(JsonObject e, const EvenimentP &ev);
void scrieIstoric(Print &o, const PunctIstoric *puncte, uint16_t n);
bool comandaPermisa(String &c);
void executaComanda(const String &c);

// imperechere: cheia casei
void imperechereInit();
bool imperecheaza(uint32_t cod, const String &clientId, String &cheieText, String &utilizator, String &parola);

// inrolare: conturile pe broker, de la serverul de conturi
bool areInrolare();
bool inroleaza(const String &clientId, String &utilizator, String &parola);
bool decripteazaComanda(const char *secret, String &comanda);

// mqtt: casa prin internet
extern SemaphoreHandle_t lacatCasa;   // datele casei, impartite cu legatura spre broker
void mqttInit();
void mqttPas();
#endif

// texte
String textEveniment(const EvenimentP &e);
String oraText(uint16_t minut);
const char *textMotiv(uint8_t motiv);
const char *textAlarma(uint8_t stare);
const char *textEnergie(uint8_t mod);

// sunet si led
void sunetPas();
void bip(uint16_t frecventa, uint16_t durata);
void ledPas();

// interfata
void uiInit();
void uiPas();
void uiEveniment(const EvenimentP &e);
void uiDateNoi();
