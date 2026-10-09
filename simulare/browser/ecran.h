// ecran.h pentru simulator: in loc de afisajul ST7796, desenam intr-o imagine
// de 480x320 pe care pagina o copiaza pe un <canvas>.
// Are doar functiile din LovyanGFX pe care le folosesc ecranele noastre,
// inclusiv fonturile VLW (aceleasi fonturi ca pe placa).

#pragma once
#include "Arduino.h"
#include <vector>

namespace lgfx {
enum textdatum_t : uint8_t {
  top_left = 0, top_center = 1, top_right = 2,
  middle_left = 4, middle_center = 5, middle_right = 6,
  bottom_left = 8, bottom_center = 9, bottom_right = 10,
  baseline_left = 16, baseline_center = 17, baseline_right = 18
};
}

class Ecran {
 public:
  static const int W = 480, H = 320;
  uint32_t px[W * H];   // RGBA, gata pentru ImageData
  int atingereX = -1, atingereY = -1;

  // culoare RGB565 (ca pe ecranul adevarat) -> RGBA
  static uint32_t rgba(uint16_t c) {
    uint32_t r = ((c >> 11) & 31) * 255 / 31, g = ((c >> 5) & 63) * 255 / 63, b = (c & 31) * 255 / 31;
    return 0xFF000000u | (b << 16) | (g << 8) | r;
  }

  void init() { fillScreen(0); }
  void setRotation(int) {}
  void setBrightness(uint8_t) {}
  void startWrite() {}
  void endWrite() {}
  void setTouchCalibrate(uint16_t *) {}
  template <class A, class B> void calibrateTouch(uint16_t *, A, B, uint8_t = 10) {}
  int getTouch(uint16_t *x, uint16_t *y) {
    if (atingereX < 0) return 0;
    *x = atingereX;
    *y = atingereY;
    return 1;
  }

  void fillScreen(uint16_t c) { fillRect(0, 0, W, H, c); }
  void fillRect(int x, int y, int w, int h, uint16_t c) {
    uint32_t col = rgba(c);
    for (int j = std::max(0, y); j < std::min(H, y + h); j++)
      for (int i = std::max(0, x); i < std::min(W, x + w); i++) px[j * W + i] = col;
  }
  void drawFastHLine(int x, int y, int w, uint16_t c) { fillRect(x, y, w, 1, c); }

  void fillSmoothRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
    r = std::min(r, std::min(w, h) / 2);
    acopera(x, y, x + w, y + h, c, [=](float a, float b) { return inRotunjit(a, b, x, y, w, h, r); });
  }
  void fillSmoothCircle(int x, int y, int r, uint16_t c) { fillSmoothRoundRect(x - r, y - r, r * 2 + 1, r * 2 + 1, r, c); }
  void drawRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
    acopera(x, y, x + w, y + h, c, [=](float a, float b) {
      return inRotunjit(a, b, x, y, w, h, r) && !inRotunjit(a, b, x + 1, y + 1, w - 2, h - 2, std::max(0, r - 1));
    });
  }
  void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c) {
    auto latura = [](float ax, float ay, float bx, float by, float px_, float py_) {
      return (bx - ax) * (py_ - ay) - (by - ay) * (px_ - ax);
    };
    acopera(std::min({x0, x1, x2}), std::min({y0, y1, y2}), std::max({x0, x1, x2}), std::max({y0, y1, y2}), c,
            [=](float a, float b) {
              float l1 = latura(x0, y0, x1, y1, a, b), l2 = latura(x1, y1, x2, y2, a, b), l3 = latura(x2, y2, x0, y0, a, b);
              return (l1 >= 0 && l2 >= 0 && l3 >= 0) || (l1 <= 0 && l2 <= 0 && l3 <= 0);
            });
  }
  void drawWideLine(float x0, float y0, float x1, float y1, float r, uint16_t c) {
    acopera(std::min(x0, x1) - r - 1, std::min(y0, y1) - r - 1, std::max(x0, x1) + r + 1, std::max(y0, y1) + r + 1, c,
            [=](float a, float b) {
              float vx = x1 - x0, vy = y1 - y0, l2 = vx * vx + vy * vy;
              float t = l2 ? ((a - x0) * vx + (b - y0) * vy) / l2 : 0;
              t = std::max(0.f, std::min(1.f, t));
              float dx = a - (x0 + t * vx), dy = b - (y0 + t * vy);
              return dx * dx + dy * dy <= r * r;
            });
  }
  void fillArc(int x, int y, int r0, int r1, float a0, float a1, uint16_t c) {
    int ri = std::min(r0, r1), ro = std::max(r0, r1);
    acopera(x - ro, y - ro, x + ro, y + ro, c, [=](float a, float b) {
      float dx = a - x, dy = b - y, d = std::sqrt(dx * dx + dy * dy);
      if (d < ri || d > ro) return false;
      float u = std::atan2(dy, dx) * 180 / PI;
      if (u < 0) u += 360;
      return u >= a0 && u <= a1;
    });
  }

  // ---- fonturi VLW
  bool loadFont(const uint8_t *f) {
    font = f;
    nr = be32(f);
    maxSus = be32(f + 16);
    maxJos = be32(f + 20);
    cod.assign(nr, 0); gh.assign(nr, 0); gw.assign(nr, 0); avans.assign(nr, 0);
    dy.assign(nr, 0); dx.assign(nr, 0); unde.assign(nr, 0);
    uint32_t ofs = 24 + nr * 28;
    for (uint32_t i = 0; i < nr; i++) {
      const uint8_t *g = f + 24 + i * 28;
      cod[i] = be32(g); gh[i] = be32(g + 4); gw[i] = be32(g + 8); avans[i] = be32(g + 12);
      dy[i] = (int32_t)be32(g + 16); dx[i] = (int32_t)be32(g + 20);
      unde[i] = ofs;
      ofs += gw[i] * gh[i];
      if (cod[i] > 0xFF || (cod[i] > 0x20 && cod[i] < 0xA0 && cod[i] != 0x7F)) {
        maxSus = std::max(maxSus, dy[i]);
        maxJos = std::max(maxJos, (int)gh[i] - dy[i]);
      }
    }
    return true;
  }
  int fontHeight() { return maxSus + maxJos; }
  void setTextDatum(lgfx::textdatum_t d) { aliniere = d; }
  void setTextColor(uint16_t f, uint16_t) { culoareText = f; }
  int textWidth(const char *s) {
    int w = 0;
    while (*s) {
      int g = litera(urmatorul(s));
      w += g < 0 ? fontHeight() * 2 / 7 : avans[g];
    }
    return w;
  }
  void drawString(const char *s, int x, int y) {
    int w = textWidth(s), h = fontHeight();
    if (aliniere & 1) x -= w / 2;
    if (aliniere & 2) x -= w;
    if (aliniere & 4) y -= h / 2;
    else if (aliniere & 8) y -= h;
    else if (aliniere & 16) y -= maxSus;
    int baza = y + maxSus;
    uint32_t c = rgba(culoareText);
    while (*s) {
      int g = litera(urmatorul(s));
      if (g < 0) { x += fontHeight() * 2 / 7; continue; }
      const uint8_t *bmp = font + unde[g];
      for (uint32_t j = 0; j < gh[g]; j++)
        for (uint32_t i = 0; i < gw[g]; i++) {
          uint8_t a = bmp[j * gw[g] + i];
          if (a) amesteca(x + dx[g] + i, baza - dy[g] + j, c, a / 255.0f);
        }
      x += avans[g];
    }
  }

 private:
  const uint8_t *font = nullptr;
  uint32_t nr = 0;
  int maxSus = 0, maxJos = 0;
  std::vector<uint32_t> cod, gh, gw, avans, unde;
  std::vector<int32_t> dy, dx;
  lgfx::textdatum_t aliniere = lgfx::top_left;
  uint16_t culoareText = 0;

  static uint32_t be32(const uint8_t *p) { return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }
  static uint32_t urmatorul(const char *&s) {
    uint8_t c = *s++;
    if (c < 0x80) return c;
    if ((c >> 5) == 6) return ((c & 31) << 6) | (*s++ & 63);
    if ((c >> 4) == 14) {
      uint32_t u = (c & 15) << 12;
      u |= (*s++ & 63) << 6;
      return u | (*s++ & 63);
    }
    s += 3;
    return '?';
  }
  int litera(uint32_t u) {
    for (uint32_t i = 0; i < nr; i++) if (cod[i] == u) return i;
    return -1;
  }
  static bool inRotunjit(float a, float b, float x, float y, float w, float h, float r) {
    if (a < x || b < y || a > x + w || b > y + h) return false;
    float cx = std::min(std::max(a, x + r), x + w - r), cy = std::min(std::max(b, y + r), y + h - r);
    return (a - cx) * (a - cx) + (b - cy) * (b - cy) <= r * r;
  }
  void amesteca(int x, int y, uint32_t c, float a) {
    if (x < 0 || y < 0 || x >= W || y >= H) return;
    uint32_t &o = px[y * W + x];
    if (a >= 1) { o = c; return; }
    uint32_t r = (c & 255) * a + (o & 255) * (1 - a);
    uint32_t g = ((c >> 8) & 255) * a + ((o >> 8) & 255) * (1 - a);
    uint32_t b = ((c >> 16) & 255) * a + ((o >> 16) & 255) * (1 - a);
    o = 0xFF000000u | (b << 16) | (g << 8) | r;
  }
  // margini netede: impartim fiecare pixel in 4x4 si vedem cat din el e acoperit
  template <class F> void acopera(int x0, int y0, int x1, int y1, uint16_t c, F inauntru) {
    uint32_t col = rgba(c);
    for (int y = std::max(0, y0); y <= std::min(H - 1, y1); y++)
      for (int x = std::max(0, x0); x <= std::min(W - 1, x1); x++) {
        int n = 0;
        for (int sy = 0; sy < 4; sy++)
          for (int sx = 0; sx < 4; sx++)
            if (inauntru(x + (sx + 0.5f) / 4, y + (sy + 0.5f) / 4)) n++;
        if (n) amesteca(x, y, col, n / 16.0f);
      }
  }
};
