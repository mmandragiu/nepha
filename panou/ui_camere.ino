// ui_camere - cate un card pentru fiecare camera: temperatura, daca e cineva,
// geamul, lumina. Butonul de jos aprinde / stinge becul de mana (Arduino
// respecta alegerea noastra o ora, apoi revine pe automat).

void desenCamere(bool tot) {
  for (uint8_t i = 0; i < NR_CAMERE; i++) {
    CameraP &c = casa.camere[i];
    uint8_t umid = i == CAMERA_DHT ? casa.umiditateAer : 0;
    String semn = zecimal(c.temp, 1) + c.lumina + c.prezenta + c.fereastra + casa.primit + umid;
    if (!schimbat(i, semn)) continue;
    int x = 12 + i * 155;
    card(x, 50, 145, 216);
    scrie(NUME_CAMERE[i], x + 14, 62, F_TEXT, C_TEXT, C_CARD);
    if (c.prezenta) pastila("e cineva", x + 14, 90, C_VERDE, C_VERDE_D);
    else pastila("liber", x + 14, 90, C_TEXT2, C_GRI_D);
    scrie(grade(c.temp), x + 14, 136, F_TITLU, C_TEXT, C_CARD, textdatum_t::middle_left);
    if (c.fereastra) scrie("geam deschis", x + 14, 164, F_MIC, C_CALD, C_CARD, textdatum_t::middle_left);
    else if (umid) scrie("umiditate " + String(umid) + "%", x + 14, 164, F_MIC, C_TEXT2, C_CARD, textdatum_t::middle_left);

    bool aprins = c.lumina > 0;
    iconBec(x + 26, 192, aprins ? C_SOARE : C_MARGINE, C_CARD);
    String lumina = "Stinsă";
    if (aprins) lumina = "Lumină " + String(c.lumina) + "%";
    scrie(lumina, x + 44, 192, F_MIC, C_TEXT2, C_CARD, textdatum_t::middle_left);
    buton(aprins ? "Stinge" : "Aprinde", x + 10, 216, 125, 40, 10 + i, !aprins);
  }
}

void atingeCamere(uint8_t id) {
  if (id < 10 || id >= 10 + NR_CAMERE) return;
  uint8_t i = id - 10;
  char cmd[20];
  snprintf(cmd, sizeof(cmd), "LUMINA %d %d", i, casa.camere[i].lumina > 0 ? 0 : 100);
  trimiteArduino(cmd);
}
