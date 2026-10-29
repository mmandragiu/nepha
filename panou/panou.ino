/*
  NEPHA - panoul casei (ESP32 cu ecran tactil de 3.5", ESP32-3248S035)
  ---------------------------------------------------------------------
  Panoul nu ia decizii: le ia Arduino ("creierul" casei). Panoul:
    - arata pe ecran ce se intampla in casa si de ce (jurnalul deciziilor)
    - trimite comenzile noastre lui Arduino (alarma, lumini, ziua de test)
    - aduce de pe internet ora si prognoza meteo si i le da lui Arduino
    - invata cat de bun e panoul solar si estimeaza productia de maine
    - serveste pagina casei si API-ul pentru aplicatie (http://nepha.local)
    - vorbeste cu aplicatia si prin internet, printr-un broker MQTT (cand telefonul
      nu e in Wi-Fi-ul casei)

  Tab-uri:
    legatura  - mesajele cu Arduino          retea   - Wi-Fi si ora
    vreme     - prognoza (open-meteo.com)     web     - pagina si API-ul
    mqtt      - API-ul prin internet          imperechere - numele panoului si cheia casei
    inrolare  - conturile pe broker (pentru panou si pentru telefoane)
    istoric   - puncte pentru grafice         texte   - jurnalul in cuvinte
    gradina   - placa din gradina (ploaia, zonele de udat), prin ESP-NOW
    sunet     - difuzor si LED                ui_*    - ecranele

  Placa: "ESP32 Dev Module", Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"
  Biblioteci (Library Manager): LovyanGFX, ArduinoJson, PubSubClient
  Cheile si parolele (Wi-Fi, inrolarea pe broker) stau in secrete.h, dupa modelul din secrete.exemplu.h
*/

#include "config.h"
#include "stare.h"
#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <LovyanGFX.hpp>
#include "ecran.h"
#include "ui.h"
#include "radacini.h"

CasaP casa;
RutinaP rutina;
RezultatTest rezultate[2];
EvenimentP jurnal[NR_JURNAL];
uint8_t nrJurnal = 0;
SemaphoreHandle_t lacatCasa;

void setup() {
  Serial.begin(115200);
  Serial.println("\nNEPHA - panoul porneste");
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  digitalWrite(PIN_LED_R, HIGH);    // LED-ul se aprinde pe LOW, deci HIGH = stins
  digitalWrite(PIN_LED_G, HIGH);
  digitalWrite(PIN_LED_B, HIGH);

  for (uint8_t i = 0; i < NR_CAMERE; i++) casa.camere[i].temp = NAN;
  casa.tempAfara = NAN;
  casa.tinta = NAN;
  casa.rata = NAN;

  lacatCasa = xSemaphoreCreateMutex();
  uiInit();          // ecranul primul, ca sa se vada ca am pornit
  legaturaInit();
  reteaInit();
  gradinaInit();     // ESP-NOW, dupa ce a pornit Wi-Fi-ul
  imperechereInit(); // dupa Wi-Fi: numerele intamplatoare pentru cheie vin din radio
  webInit();
  mqttInit();
}

void loop() {
  // datele casei se schimba doar aici; cat timp le schimbam, brokerul (alt nucleu) asteapta
  xSemaphoreTake(lacatCasa, portMAX_DELAY);
  legaturaPas();
  gradinaPas();
  istoricPas();
  xSemaphoreGive(lacatCasa);
  mqttPas();
  reteaPas();
  webPas();
  vremePas();
  invataPanou();
  uiPas();
  sunetPas();
  ledPas();
}
