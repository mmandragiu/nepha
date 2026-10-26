// web - pagina casei, servita direct de pe panou (http://nepha.local sau IP-ul de pe ecran)
//
//   /               pagina (web_pagina.h se face din web/index.html cu unelte/web_in_panou.mjs)
//   /api/stare      tot ce stie panoul, in JSON
//   /api/jurnal     ultimele decizii ale casei, in cuvinte
//   /api/istoric    punctele pentru grafice
//   /api/rutina     ce a invatat casa
//   /api/comanda    POST c=<comanda>, o trimitem lui Arduino
//   /api/imperechere POST cod=<codul de pe ecran>: telefonul primeste cheia casei (imperechere.ino)
//   /wifi           alegem reteaua casei si locul (pentru prognoza)
//
// JSON-ul il fac functiile json*() / scrieIstoric(), aceleasi si pentru broker (mqtt.ino):
// aplicatia primeste exact aceleasi campuri, pe Wi-Fi sau prin internet.

#include "web_pagina.h"

WebServer server(80);

// scrie textul intr-un String (pentru raspunsurile HTTP)
struct TextPrint : Print {
  String &s;
  TextPrint(String &s) : s(s) {}
  size_t write(uint8_t c) override { s += (char)c; return 1; }
  size_t write(const uint8_t *b, size_t n) override { s.concat((const char *)b, n); return n; }
};

static void trimiteJson(JsonDocument &doc) {
  String s;
  serializeJson(doc, s);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", s);
}

static void paginaPrincipala() {
  server.sendHeader("Content-Encoding", "gzip");
  server.sendHeader("Cache-Control", "no-cache");
  server.send_P(200, "text/html", (const char *)PAGINA_GZ, sizeof(PAGINA_GZ));
}

void jsonStare(JsonObject doc) {
  doc["server"] = idServer;
  doc["primit"] = casa.primit;
  doc["vechime"] = casa.primit ? (int32_t)((millis() - casa.primitLa) / 1000) : -1;
  doc["ora"] = oraText(casa.minut);
  doc["minut"] = casa.minut;
  doc["mod"] = casa.mod ? "smart" : "clasic";
  doc["test"] = casa.test;

  JsonArray camere = doc["camere"].to<JsonArray>();
  for (uint8_t i = 0; i < NR_CAMERE; i++) {
    JsonObject c = camere.add<JsonObject>();
    c["nume"] = NUME_CAMERE[i];
    c["temp"] = casa.camere[i].temp;
    c["lumina"] = casa.camere[i].lumina;
    c["prezenta"] = casa.camere[i].prezenta;
    c["fereastra"] = casa.camere[i].fereastra;
  }
  doc["afara"] = casa.tempAfara;
  doc["luminaNaturala"] = casa.luminaNat;

  JsonObject inc = doc["incalzire"].to<JsonObject>();
  inc["pornita"] = casa.incalzire;
  inc["tinta"] = casa.tinta;
  inc["motiv"] = textMotiv(casa.motiv);
  inc["motivCod"] = casa.motiv;
  inc["rata"] = casa.rata;

  JsonObject en = doc["energie"].to<JsonObject>();
  en["baterie"] = casa.baterie;
  en["solar"] = casa.solar;
  en["vant"] = casa.vant;
  en["consum"] = casa.consum;
  en["mod"] = textEnergie(casa.modEnergie);
  en["modCod"] = casa.modEnergie;
  en["whAzi"] = casa.whAzi;
  en["whClasic"] = casa.whClasic;
  en["mwh"] = casa.mwh;
  en["estimatSolar6h"] = productieEstimata(6);
  en["factorPanou"] = factorPanou;

  JsonObject al = doc["alarma"].to<JsonObject>();
  al["stare"] = textAlarma(casa.alarma);
  al["cod"] = casa.alarma;
  JsonArray senzori = al["senzori"].to<JsonArray>();
  for (uint8_t i = 0; i < NR_SENZORI_RF; i++) {
    JsonObject s = senzori.add<JsonObject>();
    s["nume"] = NUME_SENZORI[i];
    s["deschis"] = (bool)(casa.usi & (1 << i));
  }

  doc["gradina"]["umiditate"] = casa.sol;
  doc["gradina"]["pompa"] = casa.pompa;
  doc["priza"] = casa.priza;
  doc["senzori"] = casa.senzori;

  JsonObject v = doc["vreme"].to<JsonObject>();
  v["valida"] = vreme.valida;
  if (vreme.valida) {
    v["temp"] = vreme.temp;
    v["ploaie"] = vreme.ploaie;
    if (vreme.oraPloaie != 255) v["oraPloaie"] = vreme.oraPloaie;
    v["soare"] = vreme.soare;
    v["nori"] = vreme.nori;
    v["vant"] = vreme.vant;
  }

  JsonArray rez = doc["rezultate"].to<JsonArray>();
  for (uint8_t m = 0; m < 2; m++) {
    JsonObject r = rez.add<JsonObject>();
    r["valid"] = rezultate[m].valid;
    if (!rezultate[m].valid) continue;
    r["lumini"] = rezultate[m].lumini;
    r["incalzire"] = rezultate[m].incalzire;
    r["pompa"] = rezultate[m].pompa;
    r["priza"] = rezultate[m].priza;
    r["total"] = rezultate[m].total;
    r["mwh"] = rezultate[m].mwh;
    r["litri"] = rezultate[m].litri;
  }

  doc["retea"]["ip"] = adresaIP();
  doc["retea"]["nume"] = numeRetea();
  doc["retea"]["ap"] = modAP();
  doc["retea"]["internet"] = mqttConectat();   // panoul vorbeste cu brokerul
  doc["pretKwh"] = PRET_KWH;
}

static void apiStare() {
  JsonDocument doc;
  jsonStare(doc.to<JsonObject>());
  trimiteJson(doc);
}

void jsonEveniment(JsonObject e, const EvenimentP &ev) {
  e["ora"] = oraText(ev.minut);
  e["cod"] = ev.cod;
  e["text"] = textEveniment(ev);
}

void jsonJurnal(JsonArray a) {
  for (uint8_t i = 0; i < nrJurnal; i++) jsonEveniment(a.add<JsonObject>(), jurnal[i]);
}

static void apiJurnal() {
  JsonDocument doc;
  jsonJurnal(doc.to<JsonArray>());
  trimiteJson(doc);
}

// graficul, scris direct ca text: 288 de puncte x 7 siruri ar lua ~30 KB intr-un JsonDocument
// {"minut":[...],"tCasa":[...],"tAfara":[...],"baterie":[...],"solar":[...],"vant":[...],"consum":[...]}
void scrieIstoric(Print &o, const PunctIstoric *puncte, uint16_t n) {
  const char *nume[] = {"minut", "tCasa", "tAfara", "baterie", "solar", "vant", "consum"};
  o.print('{');
  for (uint8_t k = 0; k < 7; k++) {
    if (k) o.print(',');
    o.print('"');
    o.print(nume[k]);
    o.print("\":[");
    for (uint16_t i = 0; i < n; i++) {
      const PunctIstoric &p = puncte[i];
      if (i) o.print(',');
      switch (k) {
        case 0: o.print(p.minut); break;
        case 1: if (p.tCasa == INT16_MIN) o.print("null"); else o.print(p.tCasa / 10.0, 1); break;
        case 2: if (p.tAfara == INT16_MIN) o.print("null"); else o.print(p.tAfara / 10.0, 1); break;
        case 3: o.print(p.baterie); break;
        case 4: o.print(p.solar / 1000.0, 3); break;
        case 5: o.print(p.vant / 1000.0, 3); break;
        case 6: o.print(p.consum / 1000.0, 3); break;
      }
    }
    o.print(']');
  }
  o.print('}');
}

static void apiIstoric() {
  String s;
  s.reserve(14000);
  TextPrint o(s);
  scrieIstoric(o, istoric, nrIstoric);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", s);
}

void jsonRutina(JsonObject doc) {
  doc["valida"] = rutina.valida;
  JsonArray nume = doc["camere"].to<JsonArray>();
  JsonArray p = doc["p"].to<JsonArray>();
  for (uint8_t i = 0; i < NR_CAMERE; i++) {
    nume.add(NUME_CAMERE[i]);
    JsonArray rand = p.add<JsonArray>();
    for (uint8_t h = 0; h < 24; h++) rand.add(rutina.p[i][h]);
  }
  JsonArray zile = doc["zile"].to<JsonArray>();
  for (uint8_t h = 0; h < 24; h++) zile.add(rutina.zile[h]);
}

static void apiRutina() {
  JsonDocument doc;
  jsonRutina(doc.to<JsonObject>());
  trimiteJson(doc);
}

// primim doar comenzi "cunoscute" (si de pe Wi-Fi, si prin broker); o aducem la forma ei curata
bool comandaPermisa(String &c) {
  c.trim();
  c.toUpperCase();
  const char *permise[] = {"MOD ", "ALARMA ", "LUMINA ", "PRIZA ", "UDA", "TEST ", "TEMP ", "RUTINA", "INVATA ", "STERGE ", "USA "};
  bool ok = false;
  for (const char *p : permise) ok |= c.startsWith(p);
  return ok && c.length() <= 38;
}

// doar din loop() (acolo e legatura cu Arduino)
void executaComanda(const String &c) {
  trimiteArduino(c.c_str());
  if (c == "RUTINA STERGE") trimiteArduino("RUTINA");
}

static void apiComanda() {
  String c = server.arg("c");
  if (!comandaPermisa(c)) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }
  executaComanda(c);
  server.send(200, "application/json", "{\"ok\":true}");
}

static void apiImperechere() {
  String cheie;
  if (!imperecheaza(server.arg("cod").toInt(), cheie)) {
    server.send(403, "application/json", "{\"ok\":false}");
    return;
  }
  JsonDocument doc;
  doc["ok"] = true;
  doc["server"] = idServer;
  doc["cheie"] = cheie;
  trimiteJson(doc);
}

static const char PAGINA_WIFI[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="ro"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>NEPHA - Wi-Fi</title>
<style>
body{font-family:system-ui,sans-serif;background:#f6f1ea;color:#2b2a28;margin:0;padding:24px}
form{max-width:420px;margin:auto;background:#fff;border:1px solid #e6dfd3;border-radius:16px;padding:20px}
h1{font-size:20px;margin:0 0 4px}p{color:#8a857c;margin:0 0 16px;font-size:14px}
label{display:block;font-size:13px;color:#8a857c;margin:12px 0 4px}
input,select{width:100%;box-sizing:border-box;padding:10px;border:1px solid #e6dfd3;border-radius:10px;font-size:16px}
button{margin-top:18px;width:100%;padding:12px;border:0;border-radius:10px;background:#4c9a6a;color:#fff;font-size:16px}
.rand{display:flex;gap:8px}
</style></head><body>
<form method="post" action="/wifi">
<h1>Rețeaua casei</h1>
<p>Panoul se va conecta la rețeaua aleasă și va reporni.</p>
<label>Rețea</label><select name="ssid">%RETELE%</select>
<label>Parola</label><input name="parola" type="password">
<label>Unde e casa (pentru prognoza meteo)</label>
<div class="rand"><input name="lat" value="%LAT%"><input name="lon" value="%LON%"></div>
<button>Salvează</button>
</form></body></html>)rawliteral";

static void paginaWifi() {
  if (server.method() == HTTP_POST) {
    salveazaWifi(server.arg("ssid"), server.arg("parola"));
    float lat = server.arg("lat").toFloat(), lon = server.arg("lon").toFloat();
    if (lat != 0 && lon != 0) {
      Preferences p;
      p.begin("nepha", false);
      p.putFloat("lat", lat);
      p.putFloat("lon", lon);
      p.end();
    }
    server.send(200, "text/html; charset=utf-8", "<p style='font-family:sans-serif'>Am salvat. Panoul repornește...</p>");
    delay(1500);
    ESP.restart();
    return;
  }
  String retele;
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    String nume = WiFi.SSID(i);
    nume.replace("<", "&lt;");
    retele += "<option>" + nume + "</option>";
  }
  String pagina = FPSTR(PAGINA_WIFI);
  pagina.replace("%RETELE%", retele);
  pagina.replace("%LAT%", String(latitudine, 3));
  pagina.replace("%LON%", String(longitudine, 3));
  server.send(200, "text/html; charset=utf-8", pagina);
}

void webInit() {
  server.on("/", paginaPrincipala);
  server.on("/api/stare", apiStare);
  server.on("/api/jurnal", apiJurnal);
  server.on("/api/istoric", apiIstoric);
  server.on("/api/rutina", apiRutina);
  server.on("/api/comanda", HTTP_POST, apiComanda);
  server.on("/api/imperechere", HTTP_POST, apiImperechere);
  server.on("/wifi", paginaWifi);
  server.onNotFound([]() {
    // telefoanele conectate la NEPHA-casa ajung aici cand cauta internetul
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });
  server.begin();
}

void webPas() {
  server.handleClient();
}
