// mqtt - casa si prin internet, prin brokerul MQTT (config.h), cand telefonul nu e in
// Wi-Fi-ul casei (e pe date mobile sau in alta parte). Pe Wi-Fi ramane API-ul din web.ino.
//
// Panoul e "serverul" de pe broker. Pe trei topice (le stabileste ACL-ul brokerului):
//   TOPIC_CERERI   telefonul intreaba:  {"id":"k3j2","client":"<id telefon>","server":"<idServer>",
//                                        "cere":"stare|jurnal|istoric|rutina|comanda|privesc",
//                                        "c":"LUMINA 0 80"  sau  "secret":"..." (comanda cu PIN, criptata)}
//   TOPIC_RASPUNS  noi raspundem doar lui: {"id":"k3j2","ok":true,"date":<acelasi JSON ca pe Wi-Fi>}
//                                        sau {"id":"k3j2","ok":false,"eroare":"..."}
//   TOPIC_NOUTATI  noi anuntam pe toti:  {"tip":"stare","server":"...","date":{...}}
//                                        {"tip":"eveniment","server":"...","date":{"ora","cod","text"}}
//                                        {"tip":"online"|"offline","server":"..."}
//
// Trafic putin: anuntam doar cat timp cineva are aplicatia deschisa (ne zice "privesc" o data
// pe minut). Ce nu vrem sa ratam (usi, alarma, lumini, cine e acasa...) pleaca imediat; cifrele
// care se misca tot timpul (temperaturi, wati) cel mult o data la 30 de secunde.
//
// PIN-ul nu trece niciodata in clar prin broker: "ALARMA OPRIT <pin>" vine doar criptat, cu
// cheia casei (imperechere.ino). Contul panoului pe broker il face inrolare.ino.
//
// Legatura TLS merge pe celalalt nucleu al ESP32 (core 0), ca ecranul sa nu se blocheze cand
// ne conectam. Cu loop() (core 1) impartim datele casei sub `lacatCasa`, iar comenzile pentru
// Arduino i le dam lui loop() printr-o coada.

#include <PubSubClient.h>

static WiFiClientSecure tls;
static PubSubClient mqtt(tls);
static QueueHandle_t coadaComenzi;      // comenzi validate, pentru Arduino (le trimite loop-ul)
static QueueHandle_t coadaEvenimente;   // jurnalul, de anuntat pe broker
static volatile bool conectatBroker = false;
static volatile uint32_t privescPanaLa = 0;   // pana cand are cineva aplicatia deschisa
static uint32_t ultimaStare = 0;
static String semnUltim;
static bool stareCeruta = false;

struct ComandaM {
  char text[40];
};

bool mqttConectat() {
  return conectatBroker;
}

// "nepha/servers/{server}/clients/{client}/responses" -> "nepha/servers/a0b1c2d3e4f5/clients/k3j29dx7q1m0/responses"
static String topic(const char *model, const char *client = "") {
  String t = model;
  t.replace("{server}", idServer);
  t.replace("{client}", client);
  return t;
}

// strange textul in bucati de 512 octeti, apoi il trimite: altfel fiecare numar ar pleca
// intr-un pachet TLS separat (incet si de 10 ori mai mult trafic)
struct Tampon : Print {
  uint8_t b[512];
  size_t n = 0;
  size_t write(uint8_t c) override {
    b[n++] = c;
    if (n == sizeof(b)) gol();
    return 1;
  }
  size_t write(const uint8_t *d, size_t l) override {
    for (size_t i = 0; i < l; i++) write(d[i]);
    return l;
  }
  void gol() {
    if (n) mqtt.write(b, n);
    n = 0;
  }
};

// trimite un JSON fara sa-l mai copiem intreg in memorie
static bool publica(const String &t, JsonDocument &doc) {
  if (!mqtt.beginPublish(t.c_str(), measureJson(doc), false)) return false;
  Tampon o;
  serializeJson(doc, o);
  o.gol();
  return mqtt.endPublish();
}

// numara caracterele, ca sa stim cat de lung e mesajul inainte sa-l trimitem
struct Numaratoare : Print {
  size_t n = 0;
  size_t write(uint8_t) override { n++; return 1; }
  size_t write(const uint8_t *, size_t l) override { n += l; return l; }
};

// ---------------------------------------------------------------- noutati pentru toti

static void anuntaStarea() {
  JsonDocument doc;
  doc["tip"] = "stare";
  doc["server"] = idServer;
  xSemaphoreTake(lacatCasa, portMAX_DELAY);
  jsonStare(doc["date"].to<JsonObject>());
  xSemaphoreGive(lacatCasa);
  publica(topic(TOPIC_NOUTATI), doc);
  ultimaStare = millis();
}

// ce nu vrem sa ratam; cand se schimba ceva de aici, anuntam pe loc
static String semnImportant() {
  xSemaphoreTake(lacatCasa, portMAX_DELAY);
  String s = String(casa.primit) + casa.mod + casa.test + casa.alarma + casa.usi + casa.incalzire + casa.pompa +
             casa.priza + casa.modEnergie + '|';
  for (uint8_t i = 0; i < NR_CAMERE; i++)
    s += String(casa.camere[i].lumina) + casa.camere[i].prezenta + casa.camere[i].fereastra + ',';
  xSemaphoreGive(lacatCasa);
  return s;
}

static void anuntaCeSeSchimba() {
  if ((int32_t)(millis() - privescPanaLa) >= 0) return;   // nu se uita nimeni: nu trimitem degeaba
  String semn = semnImportant();
  uint32_t trecut = millis() - ultimaStare;
  if (stareCeruta || (semn != semnUltim && trecut >= 2000) || trecut >= 30000) {
    stareCeruta = false;
    semnUltim = semn;
    anuntaStarea();
  }
  EvenimentP e;
  while (xQueueReceive(coadaEvenimente, &e, 0) == pdTRUE) {
    JsonDocument doc;
    doc["tip"] = "eveniment";
    doc["server"] = idServer;
    jsonEveniment(doc["date"].to<JsonObject>(), e);
    publica(topic(TOPIC_NOUTATI), doc);
  }
}

// ---------------------------------------------------------------- cererile telefoanelor

static void raspunde(const String &t, const char *id, bool ok, const char *eroare = nullptr) {
  JsonDocument doc;
  doc["id"] = id;
  doc["ok"] = ok;
  if (eroare) doc["eroare"] = eroare;
  publica(t, doc);
}

// istoricul e mare (~12 KB): il copiem repede, apoi il scriem bucata cu bucata spre broker
static void raspundeIstoric(const String &t, const char *id) {
  static PunctIstoric copie[NR_ISTORIC];
  xSemaphoreTake(lacatCasa, portMAX_DELAY);
  uint16_t n = nrIstoric;
  memcpy(copie, istoric, sizeof(PunctIstoric) * n);
  xSemaphoreGive(lacatCasa);

  String inceput = String("{\"id\":\"") + id + "\",\"ok\":true,\"date\":";
  Numaratoare lung;
  scrieIstoric(lung, copie, n);
  if (!mqtt.beginPublish(t.c_str(), inceput.length() + lung.n + 1, false)) return;
  Tampon o;
  o.print(inceput);
  scrieIstoric(o, copie, n);
  o.print('}');
  o.gol();
  mqtt.endPublish();
}

static void cerere(char *, byte *date, unsigned int lung) {
  // ca text constant, ArduinoJson isi copiaza tot: buffer-ul cererii il refoloseste raspunsul
  JsonDocument c;
  if (deserializeJson(c, (const char *)date, lung)) return;
  const char *server = c["server"] | "";
  if (*server && idServer != server) return;            // e pentru alta casa
  const char *client = c["client"] | "";
  const char *id = c["id"] | "";
  // id-ul telefonului ajunge in topic: fara caractere care ar schimba topicul
  if (!*client || strlen(client) > 48 || strpbrk(client, "/+#") || strlen(id) > 16) return;
  privescPanaLa = millis() + 150000;
  String t = topic(TOPIC_RASPUNS, client);
  String ce = c["cere"] | "";

  if (ce == "privesc") {
    stareCeruta = true;                                  // ii trimitem imediat starea
    raspunde(t, id, true);
  } else if (ce == "stare" || ce == "jurnal" || ce == "rutina") {
    JsonDocument r;
    r["id"] = id;
    r["ok"] = true;
    xSemaphoreTake(lacatCasa, portMAX_DELAY);
    if (ce == "stare") jsonStare(r["date"].to<JsonObject>());
    else if (ce == "jurnal") jsonJurnal(r["date"].to<JsonArray>());
    else jsonRutina(r["date"].to<JsonObject>());
    xSemaphoreGive(lacatCasa);
    publica(t, r);
  } else if (ce == "istoric") {
    raspundeIstoric(t, id);
  } else if (ce == "comanda") {
    String cmd;
    bool criptata = c["secret"].is<const char *>();
    if (criptata) {
      if (!decripteazaComanda(c["secret"].as<const char *>(), cmd)) return raspunde(t, id, false, "secret");
    } else {
      cmd = c["c"] | "";
    }
    if (!comandaPermisa(cmd)) return raspunde(t, id, false, "comanda");
    if (cmd.startsWith("ALARMA OPRIT") && !criptata) return raspunde(t, id, false, "pin_necriptat");
    ComandaM m;
    strlcpy(m.text, cmd.c_str(), sizeof(m.text));
    if (xQueueSend(coadaComenzi, &m, 0) != pdTRUE) return raspunde(t, id, false, "ocupat");
    raspunde(t, id, true);
  } else {
    raspunde(t, id, false, "cerere");
  }
}

// ---------------------------------------------------------------- legatura cu brokerul

// contul panoului pe broker (il face inrolare.ino o data, apoi il tinem minte)
static String contUtilizator, contParola;

static void tineMinteContul() {
  Preferences p;
  p.begin("broker", false);
  p.putString("utilizator", contUtilizator);
  p.putString("parola", contParola);
  p.end();
}

static bool conecteaza() {
  if (contParola.isEmpty()) {
    if (!inroleaza("", contUtilizator, contParola)) return false;
    tineMinteContul();
  }
  tls.setCACert(RADACINI);
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setBufferSize(1024);     // cererile telefoanelor sunt mici; ce trimitem noi pleaca direct
  mqtt.setKeepAlive(30);
  mqtt.setSocketTimeout(10);
  mqtt.setCallback(cerere);
  // daca panoul dispare (pana de curent, Wi-Fi cazut), brokerul anunta singur "offline"
  String adio = String("{\"tip\":\"offline\",\"server\":\"") + idServer + "\"}";
  if (!mqtt.connect(idServer.c_str(), contUtilizator.c_str(), contParola.c_str(), topic(TOPIC_NOUTATI).c_str(), 0, false,
                    adio.c_str())) {
    Serial.printf("Broker: nu merge (cod %d)\n", mqtt.state());
    // contul nu mai e bun (sters sau resetat pe server): data viitoare cerem parola noua
    if (mqtt.state() == MQTT_CONNECT_BAD_CREDENTIALS || mqtt.state() == MQTT_CONNECT_UNAUTHORIZED) contParola = "";
    return false;
  }
  mqtt.subscribe(topic(TOPIC_CERERI).c_str());
  JsonDocument doc;
  doc["tip"] = "online";
  doc["server"] = idServer;
  publica(topic(TOPIC_NOUTATI), doc);
  Serial.println("Broker: conectat, " + idServer);
  return true;
}

static void sarcinaMqtt(void *) {
  uint32_t asteptare = 0, ultimaIncercare = 0;
  for (;;) {
    // fara internet sau fara ora (certificatul se verifica dupa data) nu are rost
    if (!online() || !areOra) {
      conectatBroker = false;
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }
    if (!mqtt.connected()) {
      conectatBroker = false;
      if (millis() - ultimaIncercare >= asteptare) {
        ultimaIncercare = millis();
        if (conecteaza()) {
          asteptare = 0;
        } else {
          asteptare = asteptare ? min(asteptare * 2, (uint32_t)300000) : 10000;   // 10 s, 20 s, ... pana la 5 minute
        }
      }
    }
    if (mqtt.connected()) {
      conectatBroker = true;
      mqtt.loop();
      anuntaCeSeSchimba();
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void mqttInit() {
  coadaComenzi = xQueueCreate(8, sizeof(ComandaM));
  coadaEvenimente = xQueueCreate(12, sizeof(EvenimentP));
  Preferences p;
  p.begin("broker", true);
  contUtilizator = p.getString("utilizator", "");
  contParola = p.getString("parola", "");
  p.end();
  if (!areInrolare() && contParola.isEmpty()) {
    Serial.println("Broker: fara cheile de inrolare in secrete.h, merg doar pe Wi-Fi");
    return;
  }
  xTaskCreatePinnedToCore(sarcinaMqtt, "mqtt", 12288, nullptr, 1, nullptr, 0);
}

// din loop(): comenzile venite prin broker pleaca spre Arduino
void mqttPas() {
  ComandaM m;
  while (xQueueReceive(coadaComenzi, &m, 0) == pdTRUE) executaComanda(String(m.text));
}

// din legatura.ino: s-a intamplat ceva in casa
void mqttEveniment(const EvenimentP &e) {
  if (coadaEvenimente && (int32_t)(millis() - privescPanaLa) < 0) xQueueSend(coadaEvenimente, &e, 0);
}
