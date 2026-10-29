// dht11.h - senzorul DHT11 (temperatura + umiditatea aerului), fara biblioteca.
// (bibliotecile DHT merg pe mai multe modele si ocupa mai mult decat ne permite Uno)
//
// Totul merge pe un singur fir:
//   1. tinem firul jos 20 ms - "masoara!"
//   2. senzorul raspunde: 80 us jos, 80 us sus
//   3. trimite 40 de biti. Fiecare bit = ~50 us jos, apoi sus: scurt (~27 us) e 0, lung (~70 us) e 1.
// Nu masuram microsecundele, ci numaram cate ture face bucla pe fiecare nivel:
// daca partea "sus" tine mai mult decat partea "jos" de dinainte, bitul e 1.
// Octetii: umiditate, (zecimi, 0 la DHT11), grade, zecimi de grad, suma de control.

#pragma once

// cate ture sta firul pe nivelul dat (0 = deloc sau prea mult, adica nu raspunde)
static uint16_t dhtPuls(uint8_t pin, uint8_t nivel) {
  uint16_t n = 0;
  while (digitalRead(pin) == nivel)
    if (++n > 400) return 0;     // ~2 ms; cel mai lung puls al senzorului e de 80 us
  return n;
}

// true daca citirea a iesit bine
static bool dhtCiteste(uint8_t pin, float &temp, uint8_t &umid) {
  uint8_t d[5] = {0, 0, 0, 0, 0};
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delay(20);
  pinMode(pin, INPUT_PULLUP);

  // 4 ms in care nu ne intrerupe nimeni (I2C si radioul asteapta putin)
  noInterrupts();
  dhtPuls(pin, HIGH);                                     // pana raspunde senzorul
  bool ok = dhtPuls(pin, LOW) && dhtPuls(pin, HIGH);      // 80 us jos, 80 us sus
  for (uint8_t i = 0; ok && i < 40; i++) {
    uint16_t jos = dhtPuls(pin, LOW);
    uint16_t sus = dhtPuls(pin, HIGH);
    if (!jos || !sus) ok = false;
    d[i / 8] = d[i / 8] << 1 | (sus > jos);
  }
  interrupts();

  if (!ok || d[0] == 0 || d[4] != (uint8_t)(d[0] + d[1] + d[2] + d[3])) return false;
  umid = d[0];
  temp = d[2] + (d[3] & 0x0F) / 10.0;
  return true;
}
