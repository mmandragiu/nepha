// ina219.h - citim INA219 direct, cu cateva randuri de cod.
// Biblioteca Adafruit trage dupa ea inca doua biblioteci si nu mai incapea
// totul in memoria lui Uno, asa ca am scris doar ce ne trebuie.
//
// INA219 masoara tensiunea pe un rezistor mic (sunt, 0.1 ohm pe modul) prin care
// trece curentul. Curent = tensiune pe sunt / rezistenta suntului.

#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "config.h"   // pentru SUNT_OHM

class Ina219 {
 public:
  explicit Ina219(uint8_t adresa) : adresa(adresa) {}

  // true cat timp vorbim cu un modul (firul I2C e al nostru, panoul asteapta)
  static volatile bool ocupat;

  // true daca modulul raspunde pe I2C
  bool porneste() {
    ocupat = true;
    Wire.beginTransmission(adresa);
    bool raspunde = Wire.endTransmission() == 0;
    ocupat = false;
    if (raspunde) scrie(0x00, 0x399F);   // configuratia implicita: 32 V, +-320 mV pe sunt, masurare continua
    return raspunde;
  }

  // tensiunea de pe magistrala (dupa sunt), in volti. 1 bit = 4 mV
  float tensiune() {
    return (citeste(0x02) >> 3) * 0.004;
  }

  // curentul in amperi. Registrul de sunt are 1 bit = 10 microvolti
  float curent() {
    int16_t brut = (int16_t)citeste(0x01);
    return brut * 0.00001 / SUNT_OHM;
  }

  float putere() {
    return tensiune() * curent();
  }

 private:
  uint8_t adresa;

  void scrie(uint8_t reg, uint16_t val) {
    ocupat = true;
    Wire.beginTransmission(adresa);
    Wire.write(reg);
    Wire.write(val >> 8);
    Wire.write(val & 0xFF);
    Wire.endTransmission();
    ocupat = false;
  }

  uint16_t citeste(uint8_t reg) {
    uint16_t v = 0;
    ocupat = true;
    Wire.beginTransmission(adresa);
    Wire.write(reg);
    if (Wire.endTransmission() == 0 && Wire.requestFrom(adresa, (uint8_t)2) == 2) {
      v = Wire.read() << 8;
      v |= Wire.read();
    }
    ocupat = false;
    return v;
  }
};

volatile bool Ina219::ocupat = false;
