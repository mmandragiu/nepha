// ds18b20.h - senzorii de temperatura DS18B20, direct cu biblioteca OneWire.
// (DallasTemperature face mult mai multe, dar ocupa ~2 KB din memoria lui Uno.)
//
// Pe un singur fir pot sta mai multi senzori; fiecare are o adresa unica de 8 octeti.
//   1. la pornire ii cautam si le tinem minte adresele
//   2. le spunem tuturor deodata "masurati!" (dureaza 0.75 s)
//   3. dupa aceea citim rezultatul de la fiecare, dupa adresa

#pragma once
#include <OneWire.h>

class Ds18b20 {
 public:
  explicit Ds18b20(uint8_t pin) : fir(pin) {}

  uint8_t adrese[4][8];
  uint8_t numar = 0;

  uint8_t cauta() {
    numar = 0;
    fir.reset_search();
    while (numar < 4 && fir.search(adrese[numar])) {
      // 0x28 = familia DS18B20; ultimul octet e suma de control
      if (adrese[numar][0] == 0x28 && crc8(adrese[numar], 7) == adrese[numar][7]) numar++;
    }
    return numar;
  }

  void masoara() {
    fir.reset();
    fir.skip();          // comanda e pentru toti senzorii de pe fir
    fir.write(0x44);     // porneste conversia
  }

  // grade C, sau NAN daca citirea n-a iesit bine
  float citeste(uint8_t i) {
    uint8_t d[9];
    if (!fir.reset()) return NAN;
    fir.select(adrese[i]);
    fir.write(0xBE);     // citeste memoria senzorului
    for (uint8_t k = 0; k < 9; k++) d[k] = fir.read();
    if (crc8(d, 8) != d[8]) return NAN;
    int16_t brut = (d[1] << 8) | d[0];
    if (brut == 0x0550) return NAN;   // 85 grade = valoarea de la pornire, inca n-a masurat
    return brut / 16.0;
  }

 private:
  OneWire fir;

  // suma de control Dallas/Maxim, calculata bit cu bit
  // (OneWire are una cu tabel, dar tabelul ocupa 256 de octeti)
  static uint8_t crc8(const uint8_t *date, uint8_t n) {
    uint8_t crc = 0;
    while (n--) {
      uint8_t b = *date++;
      for (uint8_t i = 0; i < 8; i++) {
        uint8_t amestec = (crc ^ b) & 0x01;
        crc >>= 1;
        if (amestec) crc ^= 0x8C;
        b >>= 1;
      }
    }
    return crc;
  }
};
