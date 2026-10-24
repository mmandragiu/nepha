// memorie - ce tinem minte si dupa ce se ia curentul (EEPROM)
//
//   adresa 0   setarile (temperaturi, PIN, mod)
//   adresa 32  codurile senzorilor radio, 4 octeti fiecare
//   adresa 64  rutina: in cate zile am urmarit fiecare ora (24 octeti)
//   adresa 96  rutina: ocuparea fiecarei camere (24 octeti pe camera)
//
// Folosim EEPROM.update / put, care scriu doar daca valoarea s-a schimbat
// (EEPROM-ul suporta ~100.000 de scrieri pe celula, nu vrem sa-l tocim).

#define ADR_SETARI 0
#define ADR_CODURI 32
#define ADR_ZILE   64
#define ADR_OCUPAT 96
#define SEMN_EEPROM 0x4E51

void memorieIncarca() {
  EEPROM.get(ADR_SETARI, setari);
  if (setari.semn == SEMN_EEPROM) {
    rutinaIncarca();
    return;
  }
  // prima pornire (sau am schimbat structura): luam valorile din config.h
  Serial.println(F("EEPROM gol"));
  setari.semn = SEMN_EEPROM;
  setari.confort = T_CONFORT * 10;
  setari.noapte = T_NOAPTE * 10;
  setari.eco = T_ECO * 10;
  setari.plecat = T_PLECAT * 10;
  setari.pin = PIN_ALARMA_IMPLICIT;
  setari.mod = SMART;
  salveazaSetari();
  for (uint8_t i = 0; i < NR_SENZORI_RF; i++) salveazaCodRF(i, 0);
  rutinaStergeTot();
}

void salveazaSetari() {
  EEPROM.put(ADR_SETARI, setari);
}

void salveazaCodRF(uint8_t slot, uint32_t cod) {
  EEPROM.put(ADR_CODURI + slot * 4, cod);
}

uint32_t citesteCodRF(uint8_t slot) {
  uint32_t cod;
  EEPROM.get(ADR_CODURI + slot * 4, cod);
  if (cod == 0xFFFFFFFF) cod = 0;    // EEPROM sters
  return cod;
}

void salveazaOraRutina(uint8_t ora) {
  EEPROM.update(ADR_ZILE + ora, rutinaZile[ora]);
  for (uint8_t i = 0; i < NR_CAMERE; i++) EEPROM.update(ADR_OCUPAT + i * 24 + ora, rutinaOcupat[i][ora]);
}
