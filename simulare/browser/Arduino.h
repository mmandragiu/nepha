// Arduino.h pentru simulatorul din browser.
// Imita (cat ne trebuie) mediul Arduino, ca sa putem compila acelasi cod si pentru
// Arduino Uno (casa/) si pentru panoul ESP32 (panou/) intr-o singura pagina web.
// Pinii, senzorii si legatura seriala sunt "de jucarie": ii comanda sim.cpp.

#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cmath>
#include <ctime>
#include <string>
#include <deque>
#include <algorithm>
#include <type_traits>

typedef uint8_t byte;
typedef bool boolean;

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define DEC 10
#define HEX 16
#define BIN 2
#define A0 14
#define A1 15
#define A2 16
#define A3 17
#define A4 18
#define A5 19
#define PI 3.14159265358979323846
#define SERIAL_8N1 0

#define PROGMEM
#define PSTR(s) (s)
class __FlashStringHelper;
#define F(s) (reinterpret_cast<const __FlashStringHelper *>(s))
#define FPSTR(s) (reinterpret_cast<const __FlashStringHelper *>(s))
#define pgm_read_byte(p) (*(const uint8_t *)(p))
#define pgm_read_float(p) (*(const float *)(p))
#define memcpy_P memcpy
inline int strcasecmp_P(const char *a, const char *b) {
  while (*a && *b) {
    int ca = toupper((unsigned char)*a), cb = toupper((unsigned char)*b);
    if (ca != cb) return ca - cb;
    a++;
    b++;
  }
  return toupper((unsigned char)*a) - toupper((unsigned char)*b);
}

using std::isnan;
template <class A, class B>
inline typename std::common_type<A, B>::type min(const A &a, const B &b) { return b < a ? b : a; }
template <class A, class B>
inline typename std::common_type<A, B>::type max(const A &a, const B &b) { return a < b ? b : a; }
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// timpul e acelasi pentru ambele placi
extern uint32_t simMillis;
inline uint32_t millis() { return simMillis; }
inline uint32_t micros() { return simMillis * 1000; }
inline void delay(uint32_t) {}

// pinii lui Arduino Uno (panoul are pinii lui, in sim.cpp)
extern int simIntrari[32];
extern int simAnalog[32];
extern int simIesiri[32];
inline void pinMode(uint8_t, uint8_t) {}
inline int digitalRead(uint8_t p) { return simIntrari[p]; }
inline void digitalWrite(uint8_t p, uint8_t v) { simIesiri[p] = v; }
inline int analogRead(uint8_t p) { return simAnalog[p]; }
inline void analogWrite(uint8_t p, int v) { simIesiri[p] = v; }
inline uint8_t digitalPinToInterrupt(uint8_t p) { return p == 2 ? 0 : (p == 3 ? 1 : 255); }

// ---------------------------------------------------------------- String (ca pe ESP32)
class String {
 public:
  std::string s;
  String() {}
  String(const char *c) : s(c ? c : "") {}
  String(const std::string &x) : s(x) {}
  String(const __FlashStringHelper *f) : s(reinterpret_cast<const char *>(f)) {}
  String(char c) : s(1, c) {}
  String(unsigned char v) : s(std::to_string((unsigned)v)) {}
  String(int v) : s(std::to_string(v)) {}
  String(unsigned int v) : s(std::to_string(v)) {}
  String(long v) : s(std::to_string(v)) {}
  String(unsigned long v) : s(std::to_string(v)) {}
  String(float v, unsigned int d = 2) { scrie(v, d); }
  String(double v, unsigned int d = 2) { scrie(v, d); }
  void scrie(double v, unsigned d) {
    char b[64];
    if (std::isnan(v)) snprintf(b, sizeof b, "nan");
    else snprintf(b, sizeof b, "%.*f", (int)d, v);
    s = b;
  }
  const char *c_str() const { return s.c_str(); }
  unsigned length() const { return s.size(); }
  String substring(unsigned a, unsigned b) const { return a >= s.size() ? String() : String(s.substr(a, b - a)); }
  String substring(unsigned a) const { return a >= s.size() ? String() : String(s.substr(a)); }
  int indexOf(char c, unsigned from = 0) const {
    auto p = s.find(c, from);
    return p == std::string::npos ? -1 : (int)p;
  }
  void replace(char a, char b) { std::replace(s.begin(), s.end(), a, b); }
  void remove(unsigned i) { if (i < s.size()) s.erase(i); }
  void remove(unsigned i, unsigned n) { if (i < s.size()) s.erase(i, n); }
  bool operator==(const String &o) const { return s == o.s; }
  bool operator!=(const String &o) const { return s != o.s; }
  String &operator+=(const String &o) { s += o.s; return *this; }
  String &operator+=(const char *o) { s += o; return *this; }
  String &operator+=(char c) { s += c; return *this; }
};
inline String operator+(const String &a, const String &b) { return String(a.s + b.s); }
inline String operator+(const String &a, const char *b) { return String(a.s + b); }
inline String operator+(const char *a, const String &b) { return String(std::string(a) + b.s); }
inline String operator+(const String &a, char c) { return String(a.s + c); }
template <typename T, typename std::enable_if<std::is_arithmetic<T>::value && !std::is_same<T, char>::value, int>::type = 0>
inline String operator+(const String &a, T v) {
  if (std::is_floating_point<T>::value) return a + String((double)v);
  if (std::is_same<T, bool>::value) return a + String((int)v);
  if (std::is_signed<T>::value) return a + String((long)v);
  return a + String((unsigned long)v);
}

// ---------------------------------------------------------------- Print / Stream / Serial
class Print {
 public:
  virtual size_t write(uint8_t c) = 0;
  size_t print(const char *s) { size_t n = 0; while (*s) n += write(*s++); return n; }
  size_t print(const __FlashStringHelper *s) { return print(reinterpret_cast<const char *>(s)); }
  size_t print(const String &s) { return print(s.c_str()); }
  size_t print(char c) { return write(c); }
  size_t numar(unsigned long n, int baza) {
    char buf[40];
    int i = 0;
    if (n == 0) buf[i++] = '0';
    while (n) {
      int d = n % baza;
      buf[i++] = d < 10 ? '0' + d : 'A' + d - 10;
      n /= baza;
    }
    size_t r = 0;
    while (i--) r += write(buf[i]);
    return r;
  }
  size_t print(unsigned char n, int b = DEC) { return numar(n, b); }
  size_t print(int n, int b = DEC) { return print((long)n, b); }
  size_t print(unsigned int n, int b = DEC) { return numar(n, b); }
  size_t print(long n, int b = DEC) {
    if (b == DEC && n < 0) { write('-'); return 1 + numar(-n, b); }
    return numar((unsigned long)n, b);
  }
  size_t print(unsigned long n, int b = DEC) { return numar(n, b); }
  size_t print(double x, int zecimale = 2) {
    if (std::isnan(x)) return print("nan");
    char buf[64];
    snprintf(buf, sizeof buf, "%.*f", zecimale, x);
    return print(buf);
  }
  size_t println() { return write('\n'); }
  template <typename T> size_t println(const T &v) { size_t n = print(v); return n + println(); }
  template <typename T> size_t println(const T &v, int b) { size_t n = print(v, b); return n + println(); }
  size_t printf(const char *f, ...) {
    char buf[512];
    va_list a;
    va_start(a, f);
    vsnprintf(buf, sizeof buf, f, a);
    va_end(a);
    return print(buf);
  }
};

class Stream : public Print {
 public:
  std::deque<char> in;
  virtual int available() { return (int)in.size(); }
  virtual int read() {
    if (in.empty()) return -1;
    char c = in.front();
    in.pop_front();
    return c;
  }
  void primeste(const char *s) { while (*s) in.push_back(*s++); }
};

// un port serial: ce se scrie in el se aduna in `iesire`
class SimSerial : public Stream {
 public:
  std::string iesire;
  void begin(unsigned long, int = 0, int = -1, int = -1) {}
  size_t setRxBufferSize(size_t n) { return n; }
  size_t write(uint8_t c) override { iesire.push_back((char)c); return 1; }
};

// ---------------------------------------------------------------- bibliotecile lui Uno
class TwoWire {
 public:
  void begin() {}
  void beginTransmission(uint8_t) {}
  uint8_t endTransmission(bool = true) { return 2; }   // niciun INA219 pe fir
  uint8_t requestFrom(uint8_t, uint8_t) { return 0; }
  size_t write(uint8_t) { return 1; }
  int read() { return 0; }
};
extern TwoWire Wire;

class EEPROMClass {
 public:
  uint8_t mem[1024];
  EEPROMClass() { memset(mem, 0xFF, sizeof mem); }
  uint8_t read(int a) { return mem[a]; }
  void write(int a, uint8_t v) { mem[a] = v; }
  void update(int a, uint8_t v) { mem[a] = v; }
  template <typename T> T &get(int a, T &t) { memcpy(&t, mem + a, sizeof(T)); return t; }
  template <typename T> const T &put(int a, const T &t) { memcpy(mem + a, &t, sizeof(T)); return t; }
};
extern EEPROMClass EEPROM;

class OneWire {   // fara termometre: casa foloseste modelul termic
 public:
  explicit OneWire(uint8_t) {}
  uint8_t reset() { return 0; }
  void reset_search() {}
  bool search(uint8_t *) { return false; }
  void select(const uint8_t *) {}
  void skip() {}
  void write(uint8_t, uint8_t = 0) {}
  uint8_t read() { return 0xFF; }
};

struct CodRadio {
  uint32_t valoare;
  uint8_t biti;
};
class RCSwitch {
 public:
  static std::deque<CodRadio> coada;
  void enableReceive(int) {}
  bool available() { return !coada.empty(); }
  uint32_t getReceivedValue() { return coada.front().valoare; }
  uint8_t getReceivedBitlength() { return coada.front().biti; }
  void resetAvailable() { if (!coada.empty()) coada.pop_front(); }
};

class AltSoftSerial : public SimSerial {};

class Preferences {   // panoul: nu tinem nimic minte intre rulari
 public:
  bool begin(const char *, bool = false) { return true; }
  void end() {}
  size_t getBytes(const char *, void *, size_t n) { return n; }   // "avem deja calibrarea"
  size_t putBytes(const char *, const void *, size_t n) { return n; }
  float getFloat(const char *, float d) { return d; }
  size_t putFloat(const char *, float) { return 4; }
};

// ora "de pe internet" a panoului = ora calculatorului
inline bool getLocalTime(struct tm *t, uint32_t = 0) {
  time_t acum = time(nullptr);
  *t = *localtime(&acum);
  return true;
}
