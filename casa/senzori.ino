// senzori - citim tot ce e "adevarat" in macheta
//
// Ce senzor lipseste nu e o problema: casa observa la pornire ce a gasit,
// iar ce lipseste e completat de modelul virtual (vezi energie si termostat).
// Asa programul merge si pe birou, doar cu Arduino, si in simulatorul Wokwi.

#include "ina219.h"
#include "ds18b20.h"

Ds18b20 termometre(PIN_ONEWIRE);
uint8_t camereCuSenzor = 0;     // bitul i = camera i are senzor de temperatura

Ina219 inaSolar(INA_SOLAR);
Ina219 inaVant(INA_VANT);
Ina219 inaConsum(INA_CONSUM);
Ina219 inaPriza(INA_PRIZA);

uint32_t ultimaCitireDS = 0;
uint32_t ultimaCitireINA = 0;
uint32_t ultimaCitireAnalog = 0;
float ldrFiltrat = -1, solFiltrat = -1;

void senzoriInit() {
  for (uint8_t i = 0; i < NR_CAMERE; i++) pinMode(PIN_PIR[i], INPUT);

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

  // INA219 (pe firul I2C pornit de panouInit, impreuna cu panoul)
#if defined(WIRE_HAS_TIMEOUT)
  // un fir desprins nu trebuie sa blocheze casa; 25 ms ca sa-l lasam pe panou sa termine
  Wire.setWireTimeout(25000, true);
#endif
  if (inaSolar.porneste()) casa.senzori |= SENZ_SOLAR;
  if (inaVant.porneste()) casa.senzori |= SENZ_VANT;
  if (inaConsum.porneste()) casa.senzori |= SENZ_CONSUM;
  if (inaPriza.porneste()) casa.senzori |= SENZ_PRIZA;

  Serial.print(F("Termometre: "));
  Serial.print(termometre.numar);
  Serial.print(F("  senzori: "));
  Serial.println(casa.senzori, BIN);    // vezi SENZ_* in stare.h
}

void senzoriCiteste() {
  uint32_t m = millis();

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

  // temperaturi, la 2 secunde (senzorii au masurat intre timp)
  if (termometre.numar > 0 && m - ultimaCitireDS >= 2000) {
    ultimaCitireDS = m;
    for (uint8_t i = 0; i < termometre.numar; i++) {
      float t = termometre.citeste(i);
      if (isnan(t)) continue;
      int8_t loc = LOC_SENZOR_T[i];
      if (loc >= 0 && loc < NR_CAMERE) casa.camere[loc].temp = t;
      else if (loc == -1) casa.tempAfara = t;
    }
    termometre.masoara();
  }

  // lumina naturala si umiditatea solului, filtrate ca sa nu "sara"
  if (m - ultimaCitireAnalog >= 200) {
    ultimaCitireAnalog = m;
    // map() merge si cand capetele sunt "invers" (valoare mai mica = mai multa lumina)
    float ldr = constrain(map(analogRead(PIN_LDR), LDR_INTUNERIC, LDR_SOARE, 0, 100), 0, 100);
    float sol = constrain(map(analogRead(PIN_SOL), SOL_USCAT, SOL_UD, 0, 100), 0, 100);
    ldrFiltrat = ldrFiltrat < 0 ? ldr : ldrFiltrat * 0.9 + ldr * 0.1;
    solFiltrat = solFiltrat < 0 ? sol : solFiltrat * 0.95 + sol * 0.05;
    casa.luminaNaturala = (uint8_t)(ldrFiltrat + 0.5);
    casa.gradina.umiditate = (uint8_t)(solFiltrat + 0.5);
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
  for (uint8_t i = 0; i < termometre.numar; i++) {
    Serial.print(F("termometru "));
    Serial.print(i);
    Serial.print(F(" -> loc "));
    Serial.print(LOC_SENZOR_T[i]);
    Serial.print(F(": "));
    Serial.println(termometre.citeste(i), 2);
  }
  // valorile brute, pentru calibrare (LDR_* si SOL_* in config.h)
  Serial.print(F("LDR "));
  Serial.print(analogRead(PIN_LDR));
  Serial.print(F("  sol "));
  Serial.print(analogRead(PIN_SOL));
  Serial.print(F("  baterie V "));
  Serial.println(casa.energie.vBaterie, 2);
}
