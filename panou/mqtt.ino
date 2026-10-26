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
// cheia casei (imperechere.ino).
//
// Legatura TLS merge pe celalalt nucleu al ESP32 (core 0), ca ecranul sa nu se blocheze cand
// ne conectam. Cu loop() (core 1) impartim datele casei sub `lacatCasa`, iar comenzile pentru
// Arduino i le dam lui loop() printr-o coada.

#include <PubSubClient.h>

// radacinile Let's Encrypt (ISRG Root X1 si X2): cu ele verificam ca vorbim chiar cu brokerul nostru
static const char RADACINI[] PROGMEM = R"(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw
CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg
R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00
MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT
ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw
EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW
+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9
ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T
AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI
zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW
tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1
/q4AaOeMSQ+2b1tbFfLn
-----END CERTIFICATE-----
)";

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

// "nepha/clients/{client}/responses" -> "nepha/clients/telefon1/responses"
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

static bool conecteaza() {
#if defined(MQTT_UTILIZATOR) && defined(MQTT_PAROLA)   // parolele din secrete.h
  tls.setCACert(RADACINI);
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setBufferSize(1024);     // cererile telefoanelor sunt mici; ce trimitem noi pleaca direct
  mqtt.setKeepAlive(30);
  mqtt.setSocketTimeout(10);
  mqtt.setCallback(cerere);
  // daca panoul dispare (pana de curent, Wi-Fi cazut), brokerul anunta singur "offline"
  String adio = String("{\"tip\":\"offline\",\"server\":\"") + idServer + "\"}";
  if (!mqtt.connect(idServer.c_str(), MQTT_UTILIZATOR, MQTT_PAROLA, topic(TOPIC_NOUTATI).c_str(), 0, false, adio.c_str())) {
    Serial.printf("Broker: nu merge (cod %d)\n", mqtt.state());
    return false;
  }
  mqtt.subscribe(topic(TOPIC_CERERI).c_str());
  JsonDocument doc;
  doc["tip"] = "online";
  doc["server"] = idServer;
  publica(topic(TOPIC_NOUTATI), doc);
  Serial.println("Broker: conectat, " + idServer);
  return true;
#else
  return false;
#endif
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
#if defined(MQTT_UTILIZATOR) && defined(MQTT_PAROLA)   // parolele din secrete.h
  xTaskCreatePinnedToCore(sarcinaMqtt, "mqtt", 10240, nullptr, 1, nullptr, 0);
#else
  Serial.println("Broker: fara MQTT_UTILIZATOR / MQTT_PAROLA in secrete.h, merg doar pe Wi-Fi");
#endif
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
