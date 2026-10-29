// senzori - citim tot ce e "adevarat" in macheta
//
// Ce senzor lipseste nu e o problema: casa observa la pornire ce a gasit,
// iar ce lipseste e completat de modelul virtual (vezi energie si termostat).
// Asa programul merge si pe birou, doar cu Arduino, si in simulatorul Wokwi.

#include "ina219.h"
#include "dht11.h"
#if defined(ARE_DS18B20)
#include "ds18b20.h"
Ds18b20 termometre(PIN_ONEWIRE);
#endif
uint8_t camereCuSenzor = 0;     // bitul i = camera i are senzor de temperatura

Ina219 inaSolar(INA_SOLAR);
Ina219 inaVant(INA_VANT);
Ina219 inaConsum(INA_CONSUM);
Ina219 inaPriza(INA_PRIZA);

uint32_t ultimaCitireDS = 0;
uint32_t ultimaCitireINA = 0;
uint32_t ultimaCitireAnalog = 0;
uint32_t ultimaCitireDHT = 0;
float ldrFiltrat = -1, solFiltrat = -1;

void senzoriInit() {
#if defined(ARE_PIR)
  for (uint8_t i = 0; i < NR_CAMERE; i++) pinMode(PIN_PIR[i], INPUT);
  casa.senzori |= SENZ_PIR;
#endif
#if defined(ARE_LDR)
  casa.senzori |= SENZ_LDR;
#endif
#if defined(ARE_SENZOR_SOL)
  casa.senzori |= SENZ_SOL;
#endif

#if defined(ARE_DS18B20)
  // temperaturi: unde e montat fiecare senzor gasit scrie in config.h (LOC_SENZOR_T)
  termometre.cauta();
  for (uint8_t i = 0; i < termometre.numar; i++) {
    int8_t loc = LOC_SENZOR_T[i];
    if (loc >= 0 && loc < NR_CAMERE) {
      camereCuSenzor |= 1 << loc;
      casa.senzori |= SENZ_TEMP_CAMERE;
    } else if (loc == -1) {
      casa.senzori |= SENZ_TEMP_AFARA;
    }
  }
  termometre.masoara();
  Serial.print(F("Termometre: "));
  Serial.println(termometre.numar);
#endif

  // INA219 (pe firul I2C pornit de panouInit, impreuna cu panoul)
#if defined(WIRE_HAS_TIMEOUT)
  // un fir desprins nu trebuie sa blocheze casa; 25 ms ca sa-l lasam pe panou sa termine
  Wire.setWireTimeout(25000, true);
#endif
  if (inaSolar.porneste()) casa.senzori |= SENZ_SOLAR;
  if (inaVant.porneste()) casa.senzori |= SENZ_VANT;
  if (inaConsum.porneste()) casa.senzori |= SENZ_CONSUM;
  if (inaPriza.porneste()) casa.senzori |= SENZ_PRIZA;

  Serial.print(F("Senzori: "));
  Serial.println(casa.senzori, BIN);    // vezi SENZ_* in stare.h
}

void senzoriCiteste() {
  uint32_t m = millis();

#if defined(ARE_PIR)
  // miscare - verificam la fiecare trecere, alarma trebuie sa reactioneze repede.
  // Senzorii PIR dau alarme false in primele ~30 s dupa alimentare, le ignoram.
  if (m > 30000) {
    for (uint8_t i = 0; i < NR_CAMERE; i++) {
      Camera &c = casa.camere[i];
      if (digitalRead(PIN_PIR[i]) == HIGH) {
        if (!c.prezenta) alarmaMiscare(i);
        c.ultimaMiscare = acum();
        c.prezenta = true;
      } else if (acum() - c.ultimaMiscare > 120) {
        c.prezenta = false;     // nicio miscare de 2 minute
      }
    }
  }
#endif

#if defined(ARE_DS18B20)
  // temperaturi, la 2 secunde (senzorii au masurat intre timp)
  if (termometre.numar > 0 && m - ultimaCitireDS >= 2000) {
    ultimaCitireDS = m;
    for (uint8_t i = 0; i < termometre.numar; i++) {
      float t = termometre.citeste(i);
      if (isnan(t)) continue;
      int8_t loc = LOC_SENZOR_T[i];
      if (loc == CAMERA_DHT && casa.umiditateAer) continue;     // acolo masoara DHT11
      if (loc >= 0 && loc < NR_CAMERE) casa.camere[loc].temp = t;
      else if (loc == -1) casa.tempAfara = t;
    }
    termometre.masoara();
  }
#endif

  // DHT11, la 5 secunde (mai des nu poate). Daca nu raspunde, ramane ultima valoare.
  if (m - ultimaCitireDHT >= 5000) {
    ultimaCitireDHT = m;
    float t;
    uint8_t u;
    if (dhtCiteste(PIN_DHT, t, u)) {
      // DHT11 masoara din grad in grad: netezim, altfel 20 / 21 / 20 ar parea un geam deschis
      float &tc = casa.camere[CAMERA_DHT].temp;
      tc = isnan(tc) ? t : tc * 0.8 + t * 0.2;
      casa.umiditateAer = u;
      camereCuSenzor |= 1 << CAMERA_DHT;
      casa.senzori |= SENZ_TEMP_CAMERE;
    }
  }

  // lumina naturala si umiditatea solului (de la senzori, filtrate ca sa nu "sara")
  if (m - ultimaCitireAnalog >= 200) {
    ultimaCitireAnalog = m;
    // map() merge si cand capetele sunt "invers" (valoare mai mica = mai multa lumina)
#if defined(ARE_LDR)
    float ldr = constrain(map(analogRead(PIN_LDR), LDR_INTUNERIC, LDR_SOARE, 0, 100), 0, 100);
    ldrFiltrat = ldrFiltrat < 0 ? ldr : ldrFiltrat * 0.9 + ldr * 0.1;
    casa.luminaNaturala = (uint8_t)(ldrFiltrat + 0.5);
#else
    // fara fotorezistenta: lumina unei zile de toamna (tabelul din ziua de test), mai slaba
    // daca prognoza spune ca e innorat
    float f = casa.vreme.valida ? 0.4 + casa.vreme.soare * 0.006 : 1;
    casa.luminaNaturala = dinTabel(LUMINA_AFARA, minutulZilei()) * f;
#endif
#if defined(ARE_SENZOR_SOL)
    float sol = constrain(map(analogRead(PIN_SOL), SOL_USCAT, SOL_UD, 0, 100), 0, 100);
    solFiltrat = solFiltrat < 0 ? sol : solFiltrat * 0.95 + sol * 0.05;
    casa.gradina.umiditate = (uint8_t)(solFiltrat + 0.5);
#endif
  }
}

// Productie si consum, la 0.5 s. Se apeleaza si in ziua de test: atunci
// deciziile merg pe valori virtuale, dar consumul machetei il masuram de-adevaratelea.
void senzoriEnergie() {
  uint32_t m = millis();
  if (m - ultimaCitireINA < 500) return;
  ultimaCitireINA = m;

  Energie &e = casa.energie;
  bool test = inScenariu();
  if (casa.senzori & SENZ_CONSUM) {
    e.pMasurat = fabs(inaConsum.putere());
    if (!test) {
      e.pConsum = e.pMasurat;
      e.vBaterie = inaConsum.tensiune();
    }
  }
  if (test) return;
  if (casa.senzori & SENZ_SOLAR) e.pSolar = fabs(inaSolar.putere());
  if (casa.senzori & SENZ_VANT) e.pVant = fabs(inaVant.putere());
  if (casa.senzori & SENZ_PRIZA) casa.priza.putere = fabs(inaPriza.putere());
}

// Comanda SCAN: ce vede Arduino, ca sa potrivim senzorii la montaj.
// Tineti un termometru in mana si vedeti care creste.
void senzoriScan() {
#if defined(ARE_DS18B20)
  for (uint8_t i = 0; i < termometre.numar; i++) {
    Serial.print(F("termometru "));
    Serial.print(i);
    Serial.print(F(" -> loc "));
    Serial.print(LOC_SENZOR_T[i]);
    Serial.print(F(": "));
    Serial.println(termometre.citeste(i), 2);
  }
#endif
  // valorile brute, pentru calibrare (LDR_* si SOL_* in config.h)
  Serial.print(F("LDR "));
  Serial.print(analogRead(PIN_LDR));
  Serial.print(F("  sol "));
  Serial.print(analogRead(PIN_SOL));
  Serial.print(F("  baterie V "));
  Serial.println(casa.energie.vBaterie, 2);
}
