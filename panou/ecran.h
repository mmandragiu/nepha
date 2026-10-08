// ecran.h - cum e legat ecranul pe placa ESP32-3248S035 (3.5", 480x320)
//   afisaj ST7796 pe SPI (IO14 ceas, IO13 date, IO12 citire, IO15 CS, IO2 DC)
//   lumina de fundal pe IO27
//   touch rezistiv XPT2046 pe acelasi SPI, CS pe IO33
// Daca la voi culorile ies inversate (negru in loc de alb), schimbati `invert`.

#pragma once
#include <LovyanGFX.hpp>

class Ecran : public lgfx::LGFX_Device {
  lgfx::Panel_ST7796 panou;
  lgfx::Bus_SPI magistrala;
  lgfx::Light_PWM lumina;
  lgfx::Touch_XPT2046 atingere;

 public:
  Ecran() {
    {
      auto cfg = magistrala.config();
      cfg.spi_host = SPI2_HOST;   // HSPI
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = 14;
      cfg.pin_mosi = 13;
      cfg.pin_miso = 12;
      cfg.pin_dc = 2;
      magistrala.config(cfg);
      panou.setBus(&magistrala);
    }
    {
      auto cfg = panou.config();
      cfg.pin_cs = 15;
      cfg.pin_rst = -1;
      cfg.pin_busy = -1;
      cfg.panel_width = 320;
      cfg.panel_height = 480;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = true;
      cfg.invert = false;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;     // touch-ul sta pe acelasi SPI
      panou.config(cfg);
    }
    {
      auto cfg = lumina.config();
      cfg.pin_bl = 27;
      cfg.invert = false;
      cfg.freq = 12000;
      cfg.pwm_channel = 7;
      lumina.config(cfg);
      panou.setLight(&lumina);
    }
    {
      auto cfg = atingere.config();
      cfg.x_min = 300;            // valori aproximative; la prima pornire calibram
      cfg.x_max = 3900;
      cfg.y_min = 200;
      cfg.y_max = 3800;
      cfg.pin_int = 36;
      cfg.bus_shared = true;
      cfg.offset_rotation = 0;
      cfg.spi_host = SPI2_HOST;   // HSPI
      cfg.freq = 1000000;
      cfg.pin_sclk = 14;
      cfg.pin_mosi = 13;
      cfg.pin_miso = 12;
      cfg.pin_cs = 33;
      atingere.config(cfg);
      panou.setTouch(&atingere);
    }
    setPanel(&panou);
  }
};
