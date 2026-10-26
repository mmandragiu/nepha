// imperechere - cum afla telefonul cine e panoul si cum primeste cheia casei
//
// Fiecare panou are:
//   - un nume unic (idServer, ex. "nepha-a1b2c3", din adresa MAC), ca doua case sa nu se
//     incurce pe acelasi broker
//   - o cheie secreta a casei (32 de octeti, facuta la prima pornire, tinuta in memoria panoului)
//
// Cu cheia, telefonul cripteaza PIN-ul cand dezarmeaza alarma prin internet (vezi mqtt.ino).
// Cheia o primeste o singura data, pe Wi-Fi-ul casei: pe panou, la Setari, "Leagă telefonul"
// arata un cod de 6 cifre pentru 2 minute; il scriem in aplicatie. Prin broker nu trece niciodata.

#include <esp_mac.h>
#if __has_include(<esp_random.h>)
#include <esp_random.h>
#endif
#include <mbedtls/base64.h>
#include <mbedtls/gcm.h>

String idServer;
uint8_t cheieCasa[32];

static uint32_t codImperechere = 0;   // 0 = nu asteptam niciun telefon
static uint32_t codPanaLa = 0;
static uint8_t incercariGresite = 0;

void imperechereInit() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char id[16];
  snprintf(id, sizeof(id), "nepha-%02x%02x%02x", mac[3], mac[4], mac[5]);
  idServer = id;

  Preferences p;
  p.begin("nepha", false);
  if (p.getBytes("cheie", cheieCasa, sizeof(cheieCasa)) != sizeof(cheieCasa)) {
    esp_fill_random(cheieCasa, sizeof(cheieCasa));   // cu Wi-Fi-ul pornit, numerele sunt chiar intamplatoare
    p.putBytes("cheie", cheieCasa, sizeof(cheieCasa));
  }
  p.end();
}

// apasat pe ecran: asteptam un telefon 2 minute
void incepeImperechere() {
  codImperechere = 100000 + esp_random() % 900000;
  codPanaLa = millis() + 120000;
  incercariGresite = 0;
}

// codul de afisat pe ecran (0 = nu asteptam pe nimeni)
uint32_t codDeAfisat() {
  if (codImperechere && (int32_t)(millis() - codPanaLa) >= 0) codImperechere = 0;
  return codImperechere;
}

uint8_t secundeImperechere() {
  return codDeAfisat() ? (codPanaLa - millis()) / 1000 : 0;
}

// telefonul trimite codul; daca e bun, ii dam numele panoului si cheia casei (in base64)
bool imperecheaza(uint32_t cod, String &cheieText) {
  if (!codDeAfisat()) return false;
  if (cod != codImperechere) {
    if (++incercariGresite >= 5) codImperechere = 0;   // nu lasam pe nimeni sa ghiceasca
    return false;
  }
  codImperechere = 0;                                  // un cod = un telefon
  unsigned char text[64];
  size_t lung = 0;
  mbedtls_base64_encode(text, sizeof(text), &lung, cheieCasa, sizeof(cheieCasa));
  cheieText = String((const char *)text, lung);
  return true;
}

// Comanda cu PIN venita prin internet. "secret" = base64( iv[12] | text criptat | eticheta[16] ),
// AES-256-GCM cu cheia casei, iar idServer intra in verificare (o cheie de la alta casa nu merge).
// Inauntru: "<secunde unix> <comanda>", ex. "1793012345 ALARMA OPRIT 1234".
// Refuzam mesajele mai vechi de 2 minute si pe cele vazute deja (cine retrimite un mesaj
// prins pe drum nu poate dezarma alarma).
static uint8_t ivVazute[16][12];
static uint8_t urmatorIv = 0;

bool decripteazaComanda(const char *secret, String &comanda) {
  uint8_t date[96];
  size_t lung = 0;
  if (mbedtls_base64_decode(date, sizeof(date), &lung, (const unsigned char *)secret, strlen(secret)) != 0) return false;
  if (lung < 12 + 16 + 1 || lung > 12 + 16 + 63) return false;
  const uint8_t *iv = date, *criptat = date + 12, *eticheta = date + lung - 16;
  size_t lungText = lung - 12 - 16;

  for (uint8_t i = 0; i < 16; i++)
    if (!memcmp(ivVazute[i], iv, 12)) return false;

  char text[64];
  mbedtls_gcm_context gcm;
  mbedtls_gcm_init(&gcm);
  bool ok = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, cheieCasa, 256) == 0 &&
            mbedtls_gcm_auth_decrypt(&gcm, lungText, iv, 12, (const unsigned char *)idServer.c_str(), idServer.length(),
                                     eticheta, 16, criptat, (unsigned char *)text) == 0;
  mbedtls_gcm_free(&gcm);
  if (!ok) return false;
  text[lungText] = 0;

  char *spatiu = strchr(text, ' ');
  if (!spatiu) return false;
  long long trimisLa = atoll(text);
  time_t acum = time(nullptr);
  if (acum < 1700000000 || llabs((long long)acum - trimisLa) > 120) return false;

  memcpy(ivVazute[urmatorIv], iv, 12);
  urmatorIv = (urmatorIv + 1) % 16;
  comanda = spatiu + 1;
  return true;
}
