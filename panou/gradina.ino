// gradina - placa din gradina (ESP32-S3, programul din gradina/): senzorul de ploaie si
// releele electrovalvelor (3 zone). Vorbim cu ea prin ESP-NOW: radio direct intre doua ESP32,
// fara router si fara parole. Merge si cand panoul e pe Wi-Fi-ul casei, si pe NEPHA-casa.
//
// Placa din gradina ne trimite la 2 secunde cum e: ploua sau nu, ce zona se uda acum.
// Noi ii raspundem de fiecare data; asa ne gaseste (incearca pe rand canalele Wi-Fi pana
// raspundem). In raspuns ii spunem si cand sa ude: cand Arduino porneste pompa (automat sau
// "Uda acum"), ea deschide pe rand zonele (NR_ZONE).
// Ce aflam de la ea ii spunem lui Arduino: PLOAIE 1 / PLOAIE 0.

// acelasi mesaj si in gradina/gradina.ino
struct MesajGradina {
  char semn[4];        // "NPH1", ca sa nu luam in seama alte placi
  uint8_t tip;         // 'G' de la gradina, 'P' de la panou
  uint8_t ploua;       // G: 1 = ploua
  uint8_t zona;        // G: ce zona se uda acum (0 = niciuna)
  uint8_t uda;         // P: 1 = incepe udarea
} __attribute__((packed));

GradinaP gradina;

static volatile bool mesajNou = false;
static MesajGradina primit;
static uint8_t adresaGradina[6];
static bool cerereUdare = false, pompaInainte = false;
static uint32_t ploaieTrimisaLa = 0;
static int8_t ploaieTrimisa = -1;

// vine din firul radioului, nu din loop(): doar copiem mesajul
static void laMesaj(const esp_now_recv_info_t *info, const uint8_t *date, int lung) {
  if (lung != sizeof(MesajGradina) || memcmp(date, "NPH1", 4) || date[4] != 'G') return;
  memcpy(&primit, date, sizeof(primit));
  memcpy(adresaGradina, info->src_addr, 6);
  mesajNou = true;
}

void gradinaInit() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW nu porneste");
    return;
  }
  esp_now_register_recv_cb(laMesaj);
}

static void raspunde() {
  if (!esp_now_is_peer_exist(adresaGradina)) {
    esp_now_peer_info_t p = {};
    memcpy(p.peer_addr, adresaGradina, 6);
    p.channel = 0;                  // canalul pe care suntem acum (al routerului sau al NEPHA-casa)
    p.ifidx = WIFI_IF_STA;
    esp_now_add_peer(&p);
  }
  MesajGradina m = {{'N', 'P', 'H', '1'}, 'P', 0, 0, cerereUdare};
  esp_now_send(adresaGradina, (const uint8_t *)&m, sizeof(m));
}

void gradinaPas() {
  uint32_t ms = millis();

  // Arduino a pornit pompa -> si gradina adevarata
  if (casa.pompa && !pompaInainte) cerereUdare = true;
  pompaInainte = casa.pompa;

  if (mesajNou) {
    mesajNou = false;
    bool eraAici = gradina.aici;
    gradina.aici = true;
    gradina.la = ms;
    gradina.ploua = primit.ploua;
    gradina.zona = primit.zona;
    if (gradina.zona) cerereUdare = false;     // a inceput, nu mai cerem
    raspunde();
    if (!eraAici) Serial.println("Placa din gradina e aici");
  }
  if (gradina.aici && ms - gradina.la > 30000) {
    gradina.aici = false;          // n-o mai auzim; Arduino uita singur de ploaie in 3 minute
    gradina.zona = 0;
    Serial.println("Placa din gradina nu mai raspunde");
  }
  if (!gradina.aici) cerereUdare = false;

  // ii spunem lui Arduino daca ploua: cand se schimba si o data pe minut
  if (gradina.aici && !casa.test &&
      (gradina.ploua != ploaieTrimisa || ms - ploaieTrimisaLa > 60000)) {
    ploaieTrimisa = gradina.ploua;
    ploaieTrimisaLa = ms;
    trimiteArduino(gradina.ploua ? "PLOAIE 1" : "PLOAIE 0");
  }
}
