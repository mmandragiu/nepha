/*
  NEPHA - placa din gradina (ESP32-S3)
  -------------------------------------
  Senzorul de ploaie si cele 4 electrovalve (prin relee). Placa nu ia decizii: cand sa ude
  hotaraste Arduino, iar panoul ii transmite. Ea spune doar daca ploua si deschide zonele pe rand.

  Vorbeste cu panoul prin ESP-NOW (radio direct intre doua ESP32): fara router, fara parole,
  merge si la concurs. Nu stie pe ce canal Wi-Fi e panoul, asa ca il cauta: trimite pe
  canalele 1..13 pe rand pana ii raspunde panoul, apoi ramane acolo.

  Legaturi:
    senzorul de ploaie (modulul cu comparator LM393 si potentiometru):
      VCC -> 3.3V, GND -> GND, DO -> GPIO4, AO -> GPIO1 (AO doar pentru calibrare, pe Serial)
      Pragul se regleaza din potentiometru: LED-ul de pe modul se aprinde cand e ud.
    releele (modul cu 4 relee, se activeaza pe LOW):
      VCC -> 5V dintr-o sursa separata (1.5 - 2 A), GND comun cu placa,
      IN1 -> GPIO5, IN2 -> GPIO6, IN3 -> GPIO7, IN4 -> GPIO15

  Placa: "ESP32S3 Dev Module". Nu are nevoie de biblioteci.
*/

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define PIN_PLOAIE    4
#define PIN_PLOAIE_AO 1
#define NR_ZONE 4
const uint8_t PIN_ZONA[NR_ZONE] = {5, 6, 7, 15};
#define RELEU_PORNIT LOW

#define SECUNDE_ZONA 10       // cat se uda fiecare zona (la demonstratie; intr-o gradina adevarata ~10 minute)
#define PLOAIE_TINE_S 60      // dupa ultimul strop mai zicem "ploua" un minut, sa nu sara de la o picatura la alta

// acelasi mesaj si in panou/gradina.ino
struct MesajGradina {
  char semn[4];        // "NPH1"
  uint8_t tip;         // 'G' de la noi, 'P' de la panou
  uint8_t ploua;
  uint8_t zona;        // ce zona se uda acum (0 = niciuna)
  uint8_t uda;         // de la panou: 1 = incepe udarea
} __attribute__((packed));

const uint8_t TOATA_LUMEA[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t adresaPanou[6];
uint8_t canal = 1;
bool gasit = false;
uint32_t ultimRaspuns = 0, ultimTrimis = 0;
volatile bool raspunsNou = false, cerereUdare = false;

bool aPlouat = false, ploua = false;
uint32_t ultimaPicatura = 0;
uint8_t zona = 0;             // 0 = nu udam, 1..4 = zona deschisa acum
uint32_t zonaDin = 0;

// vine din firul radioului: doar notam
void laMesaj(const esp_now_recv_info_t *info, const uint8_t *date, int lung) {
  if (lung != sizeof(MesajGradina) || memcmp(date, "NPH1", 4) || date[4] != 'P') return;
  memcpy(adresaPanou, info->src_addr, 6);
  if (date[7]) cerereUdare = true;
  raspunsNou = true;
}

void adaugaVecin(const uint8_t *adresa) {
  if (esp_now_is_peer_exist(adresa)) return;
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, adresa, 6);
  p.channel = 0;               // canalul pe care suntem acum
  p.ifidx = WIFI_IF_STA;
  esp_now_add_peer(&p);
}

void trimite() {
  MesajGradina m = {{'N', 'P', 'H', '1'}, 'G', ploua, zona, 0};
  esp_now_send(gasit ? adresaPanou : TOATA_LUMEA, (const uint8_t *)&m, sizeof(m));
}

void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < NR_ZONE; i++) {
    digitalWrite(PIN_ZONA[i], !RELEU_PORNIT);     // intai inchise, apoi iesiri (sa nu clipeasca releele)
    pinMode(PIN_ZONA[i], OUTPUT);
  }
  pinMode(PIN_PLOAIE, INPUT);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) Serial.println("ESP-NOW nu porneste");
  esp_now_register_recv_cb(laMesaj);
  adaugaVecin(TOATA_LUMEA);
  esp_wifi_set_channel(canal, WIFI_SECOND_CHAN_NONE);
  Serial.println("NEPHA gradina: caut panoul");
}

void loop() {
  uint32_t m = millis();

  // ploaia: DO e LOW cand senzorul e ud
  if (digitalRead(PIN_PLOAIE) == LOW) {
    aPlouat = true;
    ultimaPicatura = m;
  }
  bool acum = aPlouat && m - ultimaPicatura < PLOAIE_TINE_S * 1000UL;
  if (acum != ploua) {
    ploua = acum;
    Serial.printf("%s (AO %d)\n", ploua ? "Ploua" : "Nu mai ploua", analogRead(PIN_PLOAIE_AO));
    trimite();
  }

  // udarea: zonele pe rand, cate SECUNDE_ZONA fiecare
  if (cerereUdare) {
    cerereUdare = false;
    if (!zona) {
      zona = 1;
      zonaDin = m;
      Serial.println("Ud gradina");
    }
  }
  if (zona && m - zonaDin >= SECUNDE_ZONA * 1000UL) {
    zona = zona < NR_ZONE ? zona + 1 : 0;
    zonaDin = m;
  }
  for (uint8_t i = 0; i < NR_ZONE; i++) digitalWrite(PIN_ZONA[i], zona == i + 1 ? RELEU_PORNIT : !RELEU_PORNIT);

  // panoul
  if (raspunsNou) {
    raspunsNou = false;
    adaugaVecin(adresaPanou);
    ultimRaspuns = m;
    if (!gasit) Serial.printf("Am gasit panoul, pe canalul %d\n", canal);
    gasit = true;
  }
  if (gasit && m - ultimRaspuns > 10000) {
    gasit = false;              // panoul s-a oprit sau a trecut pe alt canal (alt Wi-Fi)
    Serial.println("Panoul nu mai raspunde, il caut iar");
  }
  if (gasit) {
    if (m - ultimTrimis >= 2000) {
      ultimTrimis = m;
      trimite();
    }
  } else if (m - ultimTrimis >= 300) {
    // cautam: cate un mesaj pe fiecare canal, panoul raspunde imediat daca e acolo
    ultimTrimis = m;
    canal = canal % 13 + 1;
    esp_wifi_set_channel(canal, WIFI_SECOND_CHAN_NONE);
    trimite();
  }
}
