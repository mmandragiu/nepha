// ui.h - culorile, fonturile si bucatile din care facem ecranele
#pragma once
#include <Arduino.h>
#include "stare.h"
#include "ecran.h"
using lgfx::textdatum_t;

// culori calde, de casa (RGB565)
#define RGB(r, g, b) (uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))
#define C_FUNDAL      RGB(0xF4, 0xEF, 0xE6)   // crem
#define C_CARD        RGB(0xFF, 0xFF, 0xFF)
#define C_MARGINE     RGB(0xE6, 0xDF, 0xD3)
#define C_TEXT        RGB(0x2B, 0x2A, 0x28)
#define C_TEXT2       RGB(0x8A, 0x85, 0x7C)
#define C_VERDE       RGB(0x4C, 0x9A, 0x6A)
#define C_VERDE_D     RGB(0xE3, 0xF0, 0xE7)
#define C_SOARE       RGB(0xE9, 0xA2, 0x3B)
#define C_SOARE_D     RGB(0xFB, 0xEB, 0xD0)
#define C_APA         RGB(0x4A, 0x90, 0xC2)
#define C_APA_D       RGB(0xDD, 0xEB, 0xF6)
#define C_CALD        RGB(0xE0, 0x7A, 0x5F)
#define C_CALD_D      RGB(0xF9, 0xE1, 0xDA)
#define C_ROSU        RGB(0xD6, 0x45, 0x45)
#define C_ROSU_D      RGB(0xF8, 0xDA, 0xDA)
#define C_GRI_D       RGB(0xEF, 0xEB, 0xE4)

enum EcranUI : uint8_t {
  E_ACASA, E_CAMERE, E_ENERGIE, E_RUTINA, E_GRADINA, E_ALARMA,   // cele din bara de jos
  E_JURNAL, E_SETARI, E_PIN
};

// zona de continut, intre bara de sus si bara cu file
#define SUS 44
#define JOS 268

extern Ecran tft;
extern EcranUI ecranCurent;

// fonturi (vezi fonturi.h)
enum MarimeFont : uint8_t { F_MIC, F_TEXT, F_TITLU, F_MARE };
void font(MarimeFont f);

// bucati de desen
void card(int x, int y, int w, int h, uint16_t culoare = C_CARD);
void scrie(const String &text, int x, int y, MarimeFont f, uint16_t culoare, uint16_t fundal,
           textdatum_t aliniere = textdatum_t::top_left);
uint8_t paragraf(const String &text, int x, int y, int latime, MarimeFont f, uint16_t culoare,
                 uint16_t fundal, uint8_t maxRanduri);
void pastila(const String &text, int x, int y, uint16_t culoare, uint16_t fundal, textdatum_t aliniere = textdatum_t::top_left);
void buton(const String &text, int x, int y, int w, int h, uint8_t id, bool principal = false);
void bara(int x, int y, int w, int h, float procent, uint16_t culoare, uint16_t fundal);
String zecimal(float x, uint8_t zecimale);
String grade(float x);
void adaugaZona(int x, int y, int w, int h, uint8_t id);
void stergeZona(uint8_t id);
bool schimbat(uint8_t slot, const String &semnatura);
uint16_t amesteca(uint16_t a, uint16_t b, uint8_t cat);

// iconite desenate din forme simple
void iconCasa(int cx, int cy, uint16_t c, uint16_t fundal);
void iconBec(int cx, int cy, uint16_t c, uint16_t fundal);
void iconFulger(int cx, int cy, uint16_t c);
void iconRutina(int cx, int cy, uint16_t c);
void iconPicatura(int cx, int cy, uint16_t c);
void iconScut(int cx, int cy, uint16_t c, uint16_t fundal);
void iconSoare(int cx, int cy, uint16_t c, uint16_t fundal);
void iconVant(int cx, int cy, uint16_t c);
void iconRotita(int cx, int cy, uint16_t c, uint16_t fundal);

void schimbaEcran(EcranUI e);
void deschidePin();
void calibreazaAtingerea();
float tempCasaP();
void pinRaspuns(const EvenimentP &e);
extern bool dateNoiUI;

// fiecare ecran: desen(tot) si atinge(id)
void desenAcasa(bool tot);
void atingeAcasa(uint8_t id);
void desenCamere(bool tot);
void atingeCamere(uint8_t id);
void desenEnergie(bool tot);
void atingeEnergie(uint8_t id);
void desenRutina(bool tot);
void atingeRutina(uint8_t id);
void desenGradina(bool tot);
void atingeGradina(uint8_t id);
void desenAlarma(bool tot);
void atingeAlarma(uint8_t id);
void desenPin(bool tot);
void atingePin(uint8_t id);
void desenJurnal(bool tot);
void atingeJurnal(uint8_t id);
void desenSetari(bool tot);
void atingeSetari(uint8_t id);
