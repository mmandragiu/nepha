// inrolare - de unde iau panoul si telefoanele utilizator si parola pentru broker
//
// Serverul de conturi (AUTH_URL) face cate un cont pe broker pentru fiecare panou si fiecare telefon:
//   POST /enroll/server  {"serverId"}              X-Enroll-Key: cheia panourilor
//   POST /enroll/client  {"clientId","serverId"}   X-Enroll-Key: cheia telefoanelor
// si raspunde {"username","password"}. Daca id-ul are deja cont (panoul a fost reprogramat,
// telefonul se leaga din nou), cerem o parola noua: acelasi POST cu "reset":true si cheia de resetare.
//
// Panoul se inroleaza singur o data si tine contul in memoria lui (Preferences "broker"; nu se
// sterge cand urcam alt program). Telefoanele NU se inroleaza singure: le inroleaza panoul, la
// legarea cu codul de pe ecran (imperechere.ino). Asa, cheile de inrolare stau doar pe panou,
// in secrete.h, si cont primeste doar telefonul care a fost langa panou.

#include <HTTPClient.h>

bool areInrolare() {
  return strlen(INROLARE_PANOURI) && strlen(INROLARE_TELEFOANE) && strlen(INROLARE_RESET);
}

static bool cereCont(const char *cale, const char *cheie, JsonDocument &corp, String &utilizator, String &parola) {
  WiFiClientSecure tls;
  tls.setCACert(RADACINI);
  HTTPClient http;
  if (!http.begin(tls, String(AUTH_URL) + cale)) return false;
  http.setTimeout(8000);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Enroll-Key", cheie);
  String text;
  serializeJson(corp, text);
  int cod = http.POST(text);
  bool ok = false;
  if (cod >= 200 && cod < 300) {
    JsonDocument r;
    if (!deserializeJson(r, http.getString())) {
      utilizator = r["username"] | "";
      parola = r["password"] | "";
      ok = utilizator.length() && parola.length();
    }
  }
  http.end();
  Serial.printf("Inrolare %s%s: %d\n", cale, corp["reset"] ? " (parola noua)" : "", cod);   // parola nu o scriem
  return ok;
}

// clientId gol = contul panoului; altfel contul telefonului clientId, legat de panoul nostru
bool inroleaza(const String &clientId, String &utilizator, String &parola) {
  if (!areInrolare() || !online()) return false;
  bool telefon = clientId.length();
  JsonDocument corp;
  if (telefon) corp["clientId"] = clientId;
  corp["serverId"] = idServer;
  const char *cale = telefon ? "/enroll/client" : "/enroll/server";
  if (cereCont(cale, telefon ? INROLARE_TELEFOANE : INROLARE_PANOURI, corp, utilizator, parola)) return true;
  corp["reset"] = true;   // are deja cont: cerem parola noua
  return cereCont(cale, INROLARE_RESET, corp, utilizator, parola);
}
