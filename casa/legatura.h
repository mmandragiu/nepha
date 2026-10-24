// legatura.h - vorbim cu panoul (ESP32) pe I2C, prin translatorul de nivel
//
// Firele: A4 (SDA) si A5 (SCL) de pe Arduino -> HV1 / HV2 pe translator,
// LV1 / LV2 -> IO21 / IO22 pe panou. Panoul e "seful" pe fir: de cateva zeci de
// ori pe secunda ne cere 32 de octeti (ce avem de spus) si ne scrie comenzile lui.
// Arduino raspunde la adresa 8.
//
// Pe fir merg aceleasi linii de text ca pe un port serial (S ..., E ..., ORA ...),
// asa ca restul programului foloseste legatura exact ca pe Serial.
//
// Arduino mai e si el "sef" pe acelasi fir cand citeste modulele INA219. Cat timp
// face asta (Ina219::ocupat), nu-i dam panoului nimic: o sa ne intrebe iar imediat.

#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "ina219.h"

class LegaturaI2C : public Stream {
 public:
  void begin(uint8_t adresa) {
    Wire.begin(adresa);
    Wire.onReceive(primeste);
    Wire.onRequest(cere);
  }

  int available() override { return (uint8_t)(inScris - inCitit); }
  int peek() override { return available() ? intrare[inCitit & 63] : -1; }
  int read() override {
    if (!available()) return -1;
    return intrare[inCitit++ & 63];
  }

  size_t write(uint8_t c) override {
    if (!panouAsculta()) return 1;        // nu e nimeni pe fir: nu tinem nimic
    if (sarLinia) {                       // panoul n-a mai luat nimic: aruncam restul liniei
      if (c == '\n') sarLinia = false;
      return 1;
    }
    uint32_t start = millis();
    while ((uint8_t)(outScris - outCitit) == 255) {    // plin: asteptam putin sa ia panoul
      if (millis() - start > 40) {        // panoul e ocupat: casa nu are voie sa stea dupa el
        sarLinia = c != '\n';
        return 1;
      }
      yield();
    }
    iesire[outScris] = c;
    outScris++;
    return 1;
  }

  // panoul ne-a intrebat ceva in ultimele 2 secunde?
  bool panouAsculta() {
    noInterrupts();
    uint32_t t = ultimaCerere;
    interrupts();
    return t && millis() - t < 2000;
  }

 private:
  bool sarLinia = false;
  static uint8_t iesire[256];
  static uint8_t intrare[64];
  static volatile uint8_t outScris, outCitit, inScris, inCitit;
  static volatile uint32_t ultimaCerere;

  // panoul ne-a scris o comanda (cel mult 32 de octeti odata)
  static void primeste(int n) {
    while (Wire.available()) {
      uint8_t c = Wire.read();
      if ((uint8_t)(inScris - inCitit) < 64) intrare[inScris++ & 63] = c;
    }
  }

  // panoul vrea sa stie ce avem de spus: mereu 32 de octeti, completati cu 0
  static void cere() {
    ultimaCerere = millis();
    if (Ina219::ocupat) return;
    uint8_t buf[32];
    uint8_t n = 0;
    while (n < 32 && outCitit != outScris) buf[n++] = iesire[outCitit++];
    while (n < 32) buf[n++] = 0;
    Wire.write(buf, 32);
  }
};

uint8_t LegaturaI2C::iesire[256];
uint8_t LegaturaI2C::intrare[64];
volatile uint8_t LegaturaI2C::outScris = 0, LegaturaI2C::outCitit = 0;
volatile uint8_t LegaturaI2C::inScris = 0, LegaturaI2C::inCitit = 0;
volatile uint32_t LegaturaI2C::ultimaCerere = 0;
