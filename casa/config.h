// config.h - toate setarile casei intr-un singur loc
// Daca mutati un fir pe alt pin sau vreti alt prag, aici schimbati.

#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------
// Camerele din macheta
// ---------------------------------------------------------------
#define NR_CAMERE 3          // 0 = Sufragerie, 1 = Dormitor, 2 = Bucatarie
#define CAMERA_TV 0          // in ce camera e televizorul (priza inteligenta)
#define CAMERA_DORMITOR 1

// becul (LED-ul) din fiecare camera - trebuie pini PWM (3, 5, 6, 9, 10, 11)
const uint8_t PIN_LUMINA[NR_CAMERE] = {5, 6, 9};
// senzorii de miscare PIR (HC-SR501 sau AM312)
const uint8_t PIN_PIR[NR_CAMERE] = {7, A2, A3};

// ---------------------------------------------------------------
// Iesiri
// ---------------------------------------------------------------
#define PIN_INCALZIRE 11     // MOSFET -> rezistenta care "incalzeste" casa
#define PIN_POMPA     10     // MOSFET -> pompa de irigatii
#define PIN_PRIZA     12     // releu  -> priza televizorului
#define PIN_SIRENA    13     // buzzer activ (prin tranzistor)

#define RELEU_PORNIT LOW     // modulele de releu chinezesti se activeaza pe LOW

// ---------------------------------------------------------------
// Senzori
// ---------------------------------------------------------------
#define PIN_RF       2       // receptorul 433 MHz (SYN480R). Trebuie pe D2 (intreruperea 0)
#define PIN_ONEWIRE  4       // (optional) toti senzorii DS18B20 pe acelasi fir (+ 4.7k spre 5V)
#define PIN_SOL      A0      // senzor capacitiv de umiditate a solului
#define PIN_DHT      3       // DHT11: temperatura si umiditatea aerului (in cutia ecranului)
#define CAMERA_DHT   CAMERA_DORMITOR
// Fara senzor in pamant, umiditatea solului e calculata (se usuca, creste la udat si la ploaie).
// Daca legati senzorul pe A0, decomentati linia:
// #define ARE_SENZOR_SOL
#define PIN_LDR      A1      // fotorezistenta: lumina naturala
// I2C: A4 = SDA, A5 = SCL -> modulele INA219

// Senzori DS18B20 pe un fir (D4)? Decomentati linia. Ocupa 1,5 KB: programul trece
// atunci de 30 KB si merge doar pe Uno cu bootloader nou (vezi casa.ino).
// #define ARE_DS18B20
// Senzorii DS18B20 sunt gasiti la pornire intr-o ordine fixa (dupa adresa).
// Scrieti aici unde e montat fiecare: 0..NR_CAMERE-1 = camera, -1 = afara.
// Comanda SCAN pe Serial arata ce temperatura are fiecare (tineti unul in mana).
const int8_t LOC_SENZOR_T[] = {0, 1, 2, -1};

// adresele modulelor INA219 (se schimba lipind A0 / A1 pe modul)
#define INA_SOLAR   0x40
#define INA_VANT    0x41
#define INA_CONSUM  0x44     // intre baterie si consumatorii casei
#define INA_PRIZA   0x45     // optional, pe priza televizorului
#define SUNT_OHM    0.1      // rezistenta de pe modul (R100)

// calibrare fotorezistenta: ce citeste A1 in intuneric si in plina lumina (vezi comanda SCAN).
// La modulele obisnuite cu fotorezistenta valoarea SCADE cand e mai multa lumina;
// daca la voi creste, puneti numerele invers - programul se descurca oricum.
#define LDR_INTUNERIC 1000
#define LDR_SOARE     150

// calibrare senzor sol (citire in aer uscat / in pahar cu apa)
#define SOL_USCAT 560
#define SOL_UD    270

// ---------------------------------------------------------------
// Intrerupatoarele Livolo (vezi livolo.ino)
// ---------------------------------------------------------------
#define PIN_LIVOLO 8             // DATA de la emitatorul 433 MHz (FS1000A)
#define CAMERA_LIVOLO CAMERA_DORMITOR
#define LIVOLO_TELECOMANDA 6400  // "numele" telecomenzii noastre (orice numar 1..65535)
#define LIVOLO_BUTON 0           // butonul 1 de pe telecomanda

// ---------------------------------------------------------------
// Senzori wireless 433 MHz pe usi si geamuri
// ---------------------------------------------------------------
#define NR_SENZORI_RF 4
enum TipSenzor : uint8_t { USA_INTRARE, USA, FEREASTRA };
const uint8_t TIP_SENZOR[NR_SENZORI_RF]   = {USA_INTRARE, USA, FEREASTRA, FEREASTRA};
const int8_t  CAMERA_SENZOR[NR_SENZORI_RF] = {0, 2, 1, 0};   // -1 daca nu tine de o camera
// emitatorul trimite codul cat timp usa e deschisa; daca nu-l mai auzim
// atata timp, consideram ca s-a inchis
#define TIMP_INCHIDERE_MS 2000

// ---------------------------------------------------------------
// Termostat (grade C) - valorile de pornire, se schimba din panou
// ---------------------------------------------------------------
#define T_CONFORT 21.5
#define T_NOAPTE  19.0
#define T_ECO     18.0
#define T_PLECAT  16.0
#define ORA_CULCARE 23
#define ORA_TREZIRE 6

// ---------------------------------------------------------------
// Alarma
// ---------------------------------------------------------------
#define PIN_ALARMA_IMPLICIT 1234
#define INTARZIERE_IESIRE_S 30   // cat timp avem sa iesim dupa armare
#define INTARZIERE_INTRARE_S 20  // cat timp avem sa tastam PIN-ul cand intram
#define DURATA_SIRENA_S 180

// ---------------------------------------------------------------
// Gradina
// ---------------------------------------------------------------
#define PRAG_UDARE 35        // % umiditate sub care pamantul are nevoie de apa
#define PRAG_CRITIC 15       // sub asta udam oricum
#define DURATA_POMPA_S 6     // cat merge pompa pe macheta (secunde reale)

// ---------------------------------------------------------------
// Energie
// ---------------------------------------------------------------
// Puterile la scara machetei (W) - le folosim cand nu avem INA219.
// Bateria alimenteaza doar "casa" (becuri, rezistenta, pompa, priza);
// Arduino si panoul merg de la alimentatorul lor.
#define PM_BEC        0.12
#define PM_INCALZIRE  1.0    // rezistenta de ~25 ohm la 5 V
#define PM_POMPA      1.2
#define PM_TV         0.6
#define PM_STANDBY    0.15
#define PM_SOLAR_MAX  2.0    // panoul solar in plin soare
#define BATERIE_WH    10.0   // capacitatea folosita in simulare
#define CELULE_SERIE  1      // 1 = celulele 18650 legate in paralel (3.7 V)
#define PRAG_TV_W     0.4    // masurat pe priza: peste atat televizorul e pornit, sub e standby

// Aceleasi consumatoare, dar intr-o casa adevarata (W). Din ele calculam
// cat s-ar economisi in kWh si in lei.
#define P_BEC       12
#define P_INCALZIRE 2000
#define P_TV        90
#define P_STANDBY   8
#define KWH_UDARE   0.06     // o udare: pompa 350 W, 10 minute
#define LITRI_UDARE 80

// ---------------------------------------------------------------
// Legatura cu panoul (ESP32)
// ---------------------------------------------------------------
// Pe I2C, prin translatorul de nivel: A4 = SDA, A5 = SCL (aceleasi fire ca
// modulele INA219). Panoul ne intreaba, Arduino raspunde la adresa de mai jos.
// D0/D1 raman pentru USB: putem urca programul si vedea Serial Monitor oricand.
// Pentru simularea in Wokwi decomentati linia de mai jos: atunci
// totul merge pe Serial si comenzile se scriu in Serial Monitor.
// #define FARA_PANOU
#define ADRESA_ARDUINO 8

#if defined(FARA_PANOU)
#define LEGATURA Serial
#define PERIOADA_STARE 5000       // pe Serial Monitor nu vrem o linie pe secunda
#else
#define LEGATURA legaturaPanou    // LegaturaI2C (legatura.h), definita in panou
#define PERIOADA_STARE 1000
#endif
