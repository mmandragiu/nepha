// config.h - setarile panoului (ESP32 cu ecran tactil de 3.5")
#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------
// Wi-Fi
// ---------------------------------------------------------------
// Reteaua se poate alege si fara sa umblam in cod: daca panoul nu se poate
// conecta, porneste reteaua "NEPHA-casa" (parola de mai jos). Ne conectam
// la ea cu telefonul si deschidem http://192.168.4.1/wifi
// Daca vreti totusi sa o scrieti aici, faceti un fisier secrete.h (nu se urca
// pe GitHub), dupa modelul din secrete.exemplu.h.
#if __has_include("secrete.h")
#include "secrete.h"
#endif
#ifndef WIFI_NUME
#define WIFI_NUME ""
#define WIFI_PAROLA ""
#endif
#define AP_NUME   "NEPHA-casa"
#define AP_PAROLA "casa2026"
#define NUME_RETEA "nepha"            // http://nepha.local

// ---------------------------------------------------------------
// Prin internet: brokerul MQTT, pentru cand telefonul nu e in Wi-Fi-ul casei
// ---------------------------------------------------------------
// Utilizatorul si parola panoului NU se scriu aici: le punem in secrete.h (MQTT_UTILIZATOR,
// MQTT_PAROLA). Fara ele, panoul merge doar pe Wi-Fi.
#define MQTT_SERVER "mqtt.mandragiu.eu"
#define MQTT_PORT 8883
// Topicele le stabileste ACL-ul brokerului. In ele se pot folosi:
//   {server} = numele unic al panoului (ex. nepha-a1b2c3, il vedeti la Setari)
//   {client} = id-ul telefonului care a intrebat
#define TOPIC_CERERI  "nepha/servers/{server}/requests"                     // telefoanele scriu, noi citim
#define TOPIC_RASPUNS "nepha/servers/{server}/clients/{client}/responses"  // noi raspundem fiecarui telefon
#define TOPIC_NOUTATI "nepha/servers/{server}/events/devices"              // noi anuntam ce se schimba in casa

// unde e casa (pentru prognoza meteo) - Bucuresti; se poate schimba din pagina web
#define LATITUDINE  44.43
#define LONGITUDINE 26.10

// ora Romaniei, cu trecerea la ora de vara
#define FUS_ORAR "EET-2EEST,M3.5.0/3,M10.5.0/4"

// ---------------------------------------------------------------
// Legatura cu Arduino: I2C, prin translatorul de nivel (conectorul cu IO21 / IO22)
// ---------------------------------------------------------------
#define PIN_SDA_ARDUINO 21   // IO21 -> LV1 pe translator -> HV1 -> A4 (SDA) pe Arduino
#define PIN_SCL_ARDUINO 22   // IO22 -> LV2 pe translator -> HV2 -> A5 (SCL) pe Arduino
#define ADRESA_ARDUINO 8     // la fel ca ADRESA_ARDUINO din casa/config.h

// ---------------------------------------------------------------
// Ce mai are placa pe ea
// ---------------------------------------------------------------
#define PIN_LDR_PANOU 34     // fotorezistenta de langa ecran
#define PIN_DIFUZOR   26     // amplificatorul 8002A
#define PIN_LED_R 4          // LED-ul RGB (se aprinde pe LOW)
#define PIN_LED_G 16
#define PIN_LED_B 17

// ---------------------------------------------------------------
// Casa
// ---------------------------------------------------------------
#define NR_CAMERE 3
const char *const NUME_CAMERE[NR_CAMERE] = {"Sufragerie", "Dormitor", "Bucătărie"};
#define NR_SENZORI_RF 4
const char *const NUME_SENZORI[NR_SENZORI_RF] = {"Ușa de la intrare", "Ușa spre curte", "Geam dormitor", "Geam sufragerie"};

#define PRET_KWH 1.40        // lei / kWh, ca sa aratam economia in bani
#define PM_SOLAR_MAX 2.0     // W, panoul solar al machetei in plin soare (la fel ca pe Arduino)
