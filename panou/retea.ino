// retea - Wi-Fi, ora de pe internet si reteaua proprie de rezerva
//
// 1. Incercam reteaua salvata (sau cea din secrete.h).
// 2. Daca in 20 de secunde nu merge, pornim si reteaua noastra "NEPHA-casa",
//    ca pagina sa poata fi deschisa de pe telefon oriunde (si la concurs).
//    De acolo, pe /wifi, se poate alege reteaua casei.

#include <WiFi.h>
#include <ESPmDNS.h>

Preferences setariRetea;
String wifiNume, wifiParola;
bool apPornit = false, mdnsPornit = false;
uint32_t inceputConectare = 0, ultimaReconectare = 0;
bool areOra = false;

void reteaInit() {
  setariRetea.begin("nepha", false);
  wifiNume = setariRetea.getString("ssid", WIFI_NUME);
  wifiParola = setariRetea.getString("parola", WIFI_PAROLA);
  factorPanou = setariRetea.getFloat("panou", 0.8);
  latitudine = setariRetea.getFloat("lat", LATITUDINE);
  longitudine = setariRetea.getFloat("lon", LONGITUDINE);

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(NUME_RETEA);
  if (wifiNume.length()) WiFi.begin(wifiNume.c_str(), wifiParola.c_str());
  inceputConectare = millis();
  configTzTime(FUS_ORAR, "pool.ntp.org", "time.google.com");
}

bool online() {
  return WiFi.status() == WL_CONNECTED;
}

bool modAP() {
  return apPornit;
}

String adresaIP() {
  if (online()) return WiFi.localIP().toString();
  if (apPornit) return WiFi.softAPIP().toString();
  return "-";
}

String numeRetea() {
  if (online()) return WiFi.SSID();
  if (apPornit) return AP_NUME;
  return "";
}

void salveazaWifi(const String &nume, const String &parola) {
  setariRetea.putString("ssid", nume);
  setariRetea.putString("parola", parola);
}

void reteaPas() {
  uint32_t m = millis();
  if (online()) {
    if (!mdnsPornit && MDNS.begin(NUME_RETEA)) {
      MDNS.addService("http", "tcp", 80);
      mdnsPornit = true;
      Serial.print("Conectat: http://");
      Serial.println(WiFi.localIP());
    }
  } else {
    if (!apPornit && m - inceputConectare > 20000) {
      WiFi.mode(WIFI_AP_STA);
      WiFi.softAP(AP_NUME, AP_PAROLA);
      apPornit = true;
      Serial.println("Pornesc reteaua NEPHA-casa (http://192.168.4.1)");
    }
    // mai incercam din cand in cand (rar, ca sa nu deranjam telefoanele conectate la NEPHA-casa)
    if (wifiNume.length() && m - ultimaReconectare > (apPornit ? 300000UL : 30000UL)) {
      ultimaReconectare = m;
      WiFi.begin(wifiNume.c_str(), wifiParola.c_str());
    }
  }

  struct tm t;
  if (!areOra && getLocalTime(&t, 0) && t.tm_year > 120) {
    areOra = true;
    Serial.println("Avem ora de pe internet");
  }
}
