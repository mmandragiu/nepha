/*
  NEPHA - panoul casei (ESP32 cu ecran tactil de 3.5", ESP32-3248S035)
  ---------------------------------------------------------------------
  Panoul nu ia decizii: le ia Arduino ("creierul" casei). Panoul:
    - arata pe ecran ce se intampla in casa si de ce (jurnalul deciziilor)
    - trimite comenzile noastre lui Arduino (alarma, lumini, ziua de test)
    - aduce de pe internet ora si prognoza meteo si i le da lui Arduino
    - invata cat de bun e panoul solar si estimeaza productia de maine
    - serveste pagina casei pentru telefon (http://nepha.local)

  Tab-uri:
    legatura  - mesajele cu Arduino          retea   - Wi-Fi si ora
    vreme     - prognoza (open-meteo.com)     web     - pagina si API-ul
    istoric   - puncte pentru grafice         texte   - jurnalul in cuvinte
    sunet     - difuzor si LED                ui_*    - ecranele

  Placa: "ESP32 Dev Module", Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"
  Biblioteci (Library Manager): LovyanGFX, ArduinoJson
*/

#include "config.h"
#include "stare.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <LovyanGFX.hpp>
#include "ecran.h"
#include "ui.h"

CasaP casa;
RutinaP rutina;
RezultatTest rezultate[2];
EvenimentP jurnal[NR_JURNAL];
uint8_t nrJurnal = 0;

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

  uiInit();          // ecranul primul, ca sa se vada ca am pornit
  legaturaInit();
  reteaInit();
  webInit();
}

void loop() {
  legaturaPas();
  reteaPas();
  webPas();
  vremePas();
  invataPanou();
  istoricPas();
  uiPas();
  sunetPas();
  ledPas();
}
