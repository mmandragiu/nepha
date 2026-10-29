// stare.h - "starea casei": tot ce stie Arduino despre casa intr-un moment.
// Fiecare tab citeste si scrie in structura `casa`, definita in casa.ino.

#pragma once
#include <Arduino.h>
#include "config.h"

enum ModCasa : uint8_t {
  CLASIC = 0,   // casa obisnuita: termostat fix, lumini uitate aprinse, udare la ora fixa
  SMART  = 1    // casa NEPHA: decide singura ca sa nu risipeasca
};

enum StareAlarma : uint8_t {
  ALARMA_OPRITA = 0,
  ALARMA_IESIRE,     // abia am armat, avem timp sa iesim din casa
  ALARMA_ACASA,      // armata pe noapte: doar usile si geamurile
  ALARMA_PLECAT,     // armata complet: usi, geamuri si miscare
  ALARMA_INTRARE,    // s-a deschis usa de la intrare, asteptam PIN-ul
  ALARMA_SUNA
};

enum ModEnergie : uint8_t {
  ENERGIE_NORMAL = 0,
  ENERGIE_SURPLUS,   // bateria e plina si vine soare: e momentul pentru consumatorii amanabili
  ENERGIE_ECONOMIE,  // bateria scade si nu produce nimic: reducem ce se poate
  ENERGIE_CRITIC     // doar strictul necesar
};

// coduri pentru jurnalul casei (textele sunt in panou, aici ar ocupa prea mult)
enum Eveniment : uint8_t {
  EV_PORNIT = 1,
  EV_MOD,               // a = modul nou (0 clasic, 1 smart)
  EV_PREINCALZIRE,      // a = in cate minute vine cineva, b = ora estimata
  EV_INCALZIRE_REDUSA,  // a = 0 plecati, 1 nu e nimeni, 2 noapte
  EV_FEREASTRA,         // a = camera, b = 1 dedusa din temperatura / 2 de la senzor
  EV_FEREASTRA_GATA,    // a = camera
  EV_LUMINA_STINSA,     // a = camera (era goala)
  EV_PRIZA_OPRITA,      // a = minute de standby
  EV_PRIZA_PORNITA,
  EV_UDARE,             // a = motiv (vezi gradina), b = umiditatea
  EV_UDARE_AMANATA,     // a = sansa de ploaie, b = ora ploii
  EV_ENERGIE,           // a = modul de energie, b = bateria %
  EV_ALARMA,            // a = starea noua, b = senzorul care a declansat (255 = niciunul)
  EV_USA,               // a = senzorul, b = 1 deschis / 0 inchis
  EV_SUGESTIE_ARMARE,   // casa pare goala de ceva vreme
  EV_PIN_GRESIT,        // a = incercari ramase
  EV_SENZOR_INVATAT,    // a = slotul, b = 1 reusit / 0 timp expirat
  EV_TEST               // a = 1 pornit / 0 oprit / 2 terminat, b = modul
};

struct Camera {
  float temp;               // grade C (NAN daca nu stim)
  float medieLenta;         // media temperaturii pe ultimele ~5 minute (pt fereastra deschisa)
  bool geamDeschis;         // ce spune senzorul de pe geam (sau scenariul de test)
  bool prezenta;            // a fost miscare in ultimele 2 minute
  uint32_t ultimaMiscare;   // secunda pe ceasul casei
  uint8_t lumina;           // cat de aprins e becul acum (0..100)
  uint8_t luminaTinta;      // unde vrem sa ajungem (facem trecerea lin)
  bool aprinsaClasic;       // in modul clasic: lumina aprinsa si uitata
  int8_t manual;            // -1 = automat, altfel nivelul cerut din panou
  uint32_t manualPanaLa;
  bool fereastra;           // fereastra deschisa (senzor sau dedusa)
  uint32_t fereastraPanaLa; // pana cand tinem pauza la incalzire
};

struct Energie {
  float pSolar, pVant, pConsum;  // W, la scara machetei (pConsum e cel folosit la decizii)
  float pMasurat;                // W, ce masoara INA219 pe consum (si in ziua de test)
  float vBaterie;                // V (NAN daca nu avem INA219)
  float soc;                     // % bateria
  ModEnergie mod;
  // contoare pentru azi, in Wh "de casa adevarata"
  float whLumini, whIncalzire, whPompa, whPriza;
  float whClasic;                // cat ar fi consumat o casa clasica in aceleasi conditii
  float mwhMasurat;              // energia masurata pe macheta cu INA219 (mWh)
  float litri, litriClasic;
};

struct Incalzire {
  bool pornita;
  float tinta;              // temperatura spre care mergem acum
  float rata;               // cate grade pe ora castiga casa cand incalzim (invatat)
  uint8_t motiv;            // vezi termostat
  uint32_t pornitaLa;
  float tempLaPornire;
  uint32_t preincalzirePanaLa; // odata pornita, preincalzirea tine pana vine lumea
};

struct Gradina {
  uint8_t umiditate;        // %
  bool pompa;
  uint32_t pompaPanaLa;     // millis()
  uint32_t ultimaUdare;     // secunda pe ceasul casei
  uint16_t ziuaProgramator; // programatorul clasic uda o singura data pe zi
  uint8_t motiv;
  uint16_t ziuaAnuntPloaie;
};

struct Alarma {
  StareAlarma stare;
  StareAlarma armare;       // cum a fost armata (ACASA sau PLECAT)
  uint32_t din;             // millis() de cand suntem in starea asta
  uint8_t incercari;
  uint32_t blocataPanaLa;
  bool sugerat;
};

struct Priza {
  bool pornita;
  bool tvPornit;            // televizorul e folosit, nu doar in standby
  float putere;             // W masurat (NAN daca nu avem INA219 pe priza)
  uint32_t standbyDin;
  int8_t manual;            // -1 automat, 0 oprita, 1 pornita
};

struct Vreme {
  bool valida;
  uint8_t ploaie;           // % sansa de ploaie in urmatoarele 12 ore
  uint8_t oraPloaie;        // cand (255 = nu stim)
  uint8_t soare;            // 0..100 cat soare se anunta in urmatoarele 6 ore
  float tempAfara;          // de la prognoza (daca nu avem senzor afara)
  uint32_t primitaLa;       // millis()
  bool ploua;               // senzorul de ploaie din gradina (prin panou): ploua acum
  uint32_t plouaLa;         // millis() cand am aflat
};

struct Setari {
  uint16_t semn;            // ca sa stim ca EEPROM-ul a fost scris de noi
  int16_t confort, noapte, eco, plecat;   // grade x10
  uint16_t pin;
  uint8_t mod;
};

struct Casa {
  ModCasa mod;
  Camera camere[NR_CAMERE];
  float tempAfara;
  uint8_t umiditateAer;     // % de la DHT11 (0 = nu avem senzor)
  uint8_t luminaNaturala;   // 0..100
  Energie energie;
  Incalzire incalzire;
  Gradina gradina;
  Alarma alarma;
  Priza priza;
  Vreme vreme;
  uint8_t usiDeschise;      // bitul i = senzorul RF i e deschis
  uint8_t senzori;          // ce senzori reali am gasit (vezi SENZ_*)
};

// rezultatul unei zile de test (Wh la scara unei case adevarate)
struct RezultatTest {
  float lumini, incalzire, pompa, priza, total;
  float mwh;      // masurat pe macheta (0 daca nu avem INA219 pe consum)
  float litri;
};

// biti pentru casa.senzori
#define SENZ_TEMP_CAMERE 0x01
#define SENZ_TEMP_AFARA  0x02
#define SENZ_SOLAR       0x04
#define SENZ_VANT        0x08
#define SENZ_CONSUM      0x10
#define SENZ_PRIZA       0x20
#define SENZ_SOL         0x40   // senzor in pamant (altfel umiditatea e calculata)
#define SENZ_CEAS        0x80   // ora a venit de la panou (internet)

// --- variabile folosite in mai multe tab-uri ---
extern Casa casa;
extern Setari setari;
extern uint32_t secundaDecizie;
extern uint32_t ceasSecunde;
extern uint16_t ceasViteza;
extern bool ceasSincronizat;
extern uint8_t camereCuSenzor;
extern float tempGeaman;
extern uint8_t rutinaZile[24];
extern uint8_t rutinaOcupat[NR_CAMERE][24];
extern RezultatTest rezultate[2];

// --- functii din fiecare tab (le declaram aici ca sa le vada toate tab-urile) ---

// ceas
void ceasPas();
void ceasAvanseaza(uint32_t secunde);
void ceasSeteaza(uint16_t minut);
uint32_t acum();
uint8_t oraZilei();
uint16_t minutulZilei();
uint16_t ziua();
bool eOraDeSomn(uint8_t ora);

// senzori
void senzoriInit();
void senzoriCiteste();
void senzoriEnergie();
void senzoriScan();

// usi
void usiInit();
void usiVerifica();
void usiInvata(uint8_t slot);
void usiSterge(uint8_t slot);
void usiSeteaza(uint8_t slot, bool deschis);
void usiDupaPauza();
bool senzorDeschis(uint8_t slot);

// alarma
void alarmaPas();
void alarmaSchimba(StareAlarma s, uint8_t senzor);
void alarmaArmeaza(StareAlarma s);
void alarmaDezarmeaza(uint16_t pin);
void alarmaSenzor(uint8_t slot);
void alarmaMiscare(uint8_t camera);
bool sirenaSuna();

// rutina
void rutinaPas(uint32_t dt);
uint8_t probabilitate(uint8_t camera, uint8_t ora);
uint8_t probabilitateCasa(uint8_t ora);
int16_t minutePanaVineCineva(uint16_t maxMinute);
void rutinaStergeTot();
void rutinaStergeRAM();
void rutinaIncarca();
void rutinaOraNoua();
void rutinaTrimite(Print &p);

// termostat
void decideIncalzire();
float tempCasa();
void detecteazaFerestre(uint32_t dt);
void modelTermic(uint32_t dt);
bool pasGeaman(uint32_t dt);

// lumini
void decideLumini();
bool cinevaAcasa();

// energie
void energieVirtuala(uint32_t dt);
void decideEnergie(uint32_t dt);
void decidePriza();
void contorizeaza(uint32_t dt, uint32_t dtRealMs);
void reseteazaContoare();

// gradina
void decideGradina();
void pornestePompa(uint8_t motiv);
void solPas(float &sol, uint32_t dt, bool ploua);
bool plouaAcum();
extern float solCalculat;

// livolo
void livoloPas();
void livoloLeaga();

// panou
void panouInit();
void panouPas();
void trimiteEveniment(uint8_t cod, uint8_t a, uint8_t b);
void trimiteRezultatTest(uint8_t mod);
void executaComanda(char *linie, Print &raspuns);

// memorie
void memorieIncarca();
void salveazaSetari();
void salveazaCodRF(uint8_t slot, uint32_t cod);
uint32_t citesteCodRF(uint8_t slot);
void salveazaOraRutina(uint8_t ora);

// scenariu
void scenariuPorneste(ModCasa mod);
void scenariuOpreste();
void scenariuPas(uint32_t dt);
void testRapid(Print &p);
bool inScenariu();
uint8_t stareScenariu();

// casa.ino
void decide(uint32_t dt);
void aplicaIesiri();
void camereLaZero(float temp);
