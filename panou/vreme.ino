// vreme - prognoza de pe internet (open-meteo.com, gratuit, fara cont)
//
// O luam la 30 de minute si o "rezumam" pentru Arduino in 4 numere:
//   ce sanse sunt sa ploua in urmatoarele 12 ore si la ce ora,
//   cat soare vine in urmatoarele 6 ore, cate grade sunt afara.
// Tot aici invatam cat de bun e panoul nostru solar: comparam ce produce
// cu cat soare spune prognoza ca este, si folosim raportul ca sa estimam
// cat vom produce in orele urmatoare.

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>

VremeP vreme;
float factorPanou = 0.8;
uint32_t ultimaIncercareVreme = 0;
uint32_t ultimaInvatarePanou = 0;
float latitudine = LATITUDINE, longitudine = LONGITUDINE;

static bool iaVremea() {
  WiFiClientSecure client;
  client.setInsecure();     // nu verificam certificatul - e doar prognoza
  HTTPClient http;
  char url[300];
  snprintf(url, sizeof(url),
           "https://api.open-meteo.com/v1/forecast?latitude=%.3f&longitude=%.3f"
           "&current=temperature_2m,wind_speed_10m,cloud_cover"
           "&hourly=precipitation_probability,shortwave_radiation&forecast_hours=12&timezone=auto",
           latitudine, longitudine);
  if (!http.begin(client, url)) return false;
  http.setTimeout(8000);
  int cod = http.GET();
  if (cod != 200) {
    Serial.printf("vreme: HTTP %d\n", cod);
    http.end();
    return false;
  }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getString());
  http.end();
  if (err) {
    Serial.printf("vreme: JSON %s\n", err.c_str());
    return false;
  }

  vreme.temp = doc["current"]["temperature_2m"] | NAN;
  vreme.vant = doc["current"]["wind_speed_10m"] | 0.0f;
  vreme.nori = doc["current"]["cloud_cover"] | 0;
  JsonArray ore = doc["hourly"]["time"];
  JsonArray ploaie = doc["hourly"]["precipitation_probability"];
  JsonArray rad = doc["hourly"]["shortwave_radiation"];

  uint8_t maxPloaie = 0, oraMax = 255;
  float sumaSoare = 0;
  for (uint8_t i = 0; i < 12 && i < ploaie.size(); i++) {
    uint8_t p = ploaie[i] | 0;
    if (p > maxPloaie) {
      maxPloaie = p;
      const char *t = ore[i] | "";
      oraMax = strlen(t) >= 13 ? atoi(t + 11) : 255;    // "2026-10-08T16:00"
    }
    vreme.radiatie[i] = rad[i] | 0.0f;
    if (i < 6) sumaSoare += vreme.radiatie[i];
  }
  vreme.ploaie = maxPloaie;
  vreme.oraPloaie = maxPloaie >= 30 ? oraMax : 255;
  vreme.soare = constrain(sumaSoare / 6 / 6, 0, 100);     // ~600 W/m2 = soare plin
  vreme.valida = true;
  vreme.la = millis();
  ultimaVremeTrimisa = 0;     // i-o trimitem lui Arduino imediat
  Serial.printf("vreme: %.1f C, ploaie %d%% la %d, soare %d%%\n", vreme.temp, vreme.ploaie, vreme.oraPloaie, vreme.soare);
  return true;
}

void vremePas() {
  if (!online()) return;
  uint32_t m = millis();
  uint32_t pauza = vreme.valida ? 30UL * 60000 : 60000;    // daca n-a mers, reincercam peste un minut
  if (ultimaIncercareVreme && m - ultimaIncercareVreme < pauza) return;
  ultimaIncercareVreme = m;
  iaVremea();
}

// Cat de bun e panoul: producem X, iar dupa soarele din prognoza ar fi trebuit Y.
// Tinem o medie a raportului X / Y (invatam incet, un nor nu strica media).
void invataPanou() {
  if (millis() - ultimaInvatarePanou < 300000UL) return;      // o data la 5 minute
  ultimaInvatarePanou = millis();
  if (!vreme.valida || !(casa.senzori & 0x04) || casa.test) return;   // fara senzor pe panou nu avem ce invata
  float asteptat = PM_SOLAR_MAX * vreme.radiatie[0] / 1000.0;
  if (vreme.radiatie[0] < 150 || asteptat < 0.05) return;           // prea putin soare ca sa conteze
  float raport = constrain(casa.solar / asteptat, 0.05, 1.5);
  factorPanou = factorPanou * 0.9 + raport * 0.1;
  Preferences p;
  p.begin("nepha", false);
  p.putFloat("panou", factorPanou);
  p.end();
}

// Wh pe care estimam ca le va produce panoul in urmatoarele `ore` ore
float productieEstimata(uint8_t ore) {
  if (!vreme.valida) return NAN;
  float wh = 0;
  for (uint8_t i = 0; i < ore && i < 12; i++) wh += PM_SOLAR_MAX * vreme.radiatie[i] / 1000.0 * factorPanou;
  return wh;
}
