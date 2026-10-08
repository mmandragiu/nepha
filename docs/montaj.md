# Montajul

Tot ce ține de pini e și în [`casa/config.h`](../casa/config.h) și [`panou/config.h`](../panou/config.h).
Dacă mutați un fir, schimbați acolo și atât.

## Arduino Uno — „creierul”

| Pin | Ce e legat | Observații |
|---|---|---|
| D2 | receptor 433 MHz SYN480R (DATA) | trebuie D2: biblioteca RCSwitch folosește întreruperea 0 |
| D3 | bec (LED) Bucătărie | PWM, prin rezistență 220 Ω |
| D4 | firul comun al senzorilor DS18B20 | rezistență 4,7 kΩ între D4 și 5 V |
| D5 | bec Sufragerie | PWM |
| D6 | bec Dormitor | PWM |
| D7 | PIR Sufragerie | |
| D8 | ← TX de la panou (IO22) | AltSoftSerial, pin fix |
| D9 | → RX la panou (IO35) | AltSoftSerial, pin fix; **prin divizor sau translator de nivel** |
| D10 | MOSFET pompă | |
| D11 | MOSFET încălzire (rezistența) | |
| D12 | releu priza TV | releele chinezești se activează pe LOW (`RELEU_PORNIT`) |
| D13 | buzzer activ (sirena) | prin tranzistor sau modul de buzzer |
| A0 | senzor capacitiv umiditate sol (AOUT) | |
| A1 | fotorezistența (modul, ieșirea AO) | |
| A2 | PIR Dormitor | |
| A3 | PIR Bucătărie | |
| A4 / A5 | SDA / SCL pentru INA219 | toate modulele pe aceleași două fire |
| D0 / D1 | USB | rămân libere, ca să putem urca programul și citi Serial Monitor fără să scoatem fire |

Toți pinii lui Uno sunt folosiți. Dacă vreți încă ceva (de exemplu o lumină afară), cel mai simplu
e un senzor de lumină BH1750 pe I2C în loc de fotorezistență: eliberează A1.

### Consumatorii mari: prin MOSFET, nu direct din pin

Un pin de Arduino dă cel mult ~20 mA. LED-urile de cameră merg direct (cu rezistență), dar
pompa, rezistența de încălzire și benzile LED trebuie comandate printr-un MOSFET „logic level”
(IRLZ44N, sau un modul cu MOSFET):

```
 pin Arduino ── 220 Ω ── Gate          +5 V (de la baterie / step-up)
                          │                │
                 10 kΩ ── GND         consumator (pompa)
                                           │
                                      Drain ┘   Source ── GND
```

La pompă (motor) puneți și o diodă 1N4007 în paralel cu ea, invers (catodul la +), ca să nu
se întoarcă vârful de tensiune în MOSFET.

### Încălzirea pe machetă

Pe machetă „caloriferul” e o rezistență de ~25 Ω / 2 W la 5 V (încălzește puțin, consumă ~1 W).
Dacă vreți ca temperatura să crească de-adevăratelea pe senzor, lipiți senzorul DS18B20 din
sufragerie aproape de rezistență. Camerele fără senzor folosesc oricum modelul termic din program.

## Senzorii de temperatură DS18B20

Toți pe același fir (D4), fiecare cu VCC la 5 V și GND. O singură rezistență de 4,7 kΩ între
fir și 5 V. Pentru afară, varianta în capsulă metalică, cu cablu.

La pornire, Arduino îi găsește într-o ordine fixă. Ca să știe care e în ce cameră:
1. scrieți `SCAN` în Serial Monitor și țineți câte un senzor în mână;
2. completați în `config.h` lista `LOC_SENZOR_T` — de exemplu `{2, 0, 1, -1}` înseamnă: primul
   găsit e în Bucătărie, al doilea în Sufragerie, al treilea în Dormitor, al patrulea afară.

## INA219 — cât se produce și cât se consumă

Fiecare modul măsoară curentul care trece prin el (între VIN+ și VIN−) și tensiunea. Adresele se
schimbă lipind punțile A0/A1 de pe modul:

| Modul | Adresa | Unde se pune |
|---|---|---|
| Solar | 0x40 (nimic lipit) | între panoul solar și modulul de încărcare |
| Vânt | 0x41 (A0 lipit) | între turbină (după redresor) și încărcare |
| Consum | 0x44 (A1 lipit) | între baterie și step-up-ul care alimentează casa — măsoară și tensiunea bateriei |
| Priză TV | 0x45 (A0 și A1 lipite) | pe firul de + al prizei / al consumatorului „TV” (opțional) |

Ce modul lipsește nu e o problemă: programul vede la pornire ce a găsit și completează cu un
model (de exemplu producția solară o estimează din fotorezistență).

## Energia: panou, turbină, baterie

```
 panou solar 6 V ── INA219 0x40 ── diodă ─┐
                                          ├── modul încărcare (CN3791 sau TP4056) ── 18650 (în paralel)
 turbină ── redresor ── step-up ── INA219 0x41 ── diodă ─┘                               │
                                                                        INA219 0x44 ── step-up 5 V ── casa
```

- **TP4056** vrea 5 V la intrare. Un panou de 6 V merge direct doar prin modulul **CN3791**
  (e făcut pentru panouri solare); altfel puneți un step-down la 5 V înainte de TP4056.
- **Turbina** dă o tensiune care variază cu vântul (și alternativă, dacă generatorul e un motor
  pas cu pas): redresor în punte, apoi un modul step-up/step-down reglat pe 5 V.
- **Diodele Schottky** (1N5819) nu lasă bateria să se descarce înapoi în panou sau turbină.
- Celulele 18650 le legăm **în paralel** (3,7 V), cu protecție (BMS). Dacă le legați în serie,
  schimbați `CELULE_SERIE` în `config.h`.
- Arduino și panoul merg de la alimentatorul lor (priză + acumulatorul de urgență din panou);
  bateria solară alimentează doar „casa”: becurile, rezistența, pompa, priza.

La turbină, nu vă speriați de cifre: o turbină Savonius de balcon printată 3D dă câteva sute de
mW la vânt bun. Pentru demonstrație, un ventilator în fața ei. Important e că sistemul o
măsoară și o folosește în decizii.

## Legătura Arduino ↔ panou (conectorul P3 de pe placa ESP32)

| Panou (P3) | Arduino | |
|---|---|---|
| IO22 (TX) | D8 (RX) | 3,3 V intră direct în Arduino, merge |
| IO35 (RX) | D9 (TX) | **5 V nu au voie să intre în ESP32**: translator de nivel sau divizor 1 kΩ / 2 kΩ |
| GND | GND | masa comună, obligatoriu |

Divizorul: D9 ── 1 kΩ ── IO35 ── 2 kΩ ── GND. Cu translatorul de nivel (cel cu 4 canale, BSS138):
LV la 3,3 V de pe panou, HV la 5 V de pe Arduino, și treceți ambele fire prin el.

Viteza: 9600 baud. Pe Uno folosim AltSoftSerial (D8/D9) și nu pinii 0/1, ca să putem urca
programul și să vedem Serial Monitor fără să deconectăm panoul.

## Senzorii wireless de pe uși și geamuri (433 MHz)

Receptorul **SYN480R** stă lângă Arduino (VCC 5 V, GND, DATA la D2). Pe receptor și pe fiecare
emițător, câte o antenă: un fir de 17,3 cm.

Pe fiecare ușă / geam, un emițător care trimite un cod unic cât timp ușa e deschisă. Variante,
de la cea mai simplă:

1. **Senzori de ușă gata făcuți, 433 MHz** (15-30 lei). Merg direct.
2. **Telecomandă 433 MHz cu EV1527** (de tip breloc), desfăcută, cu contactul magnetic lipit pe
   un buton. Atenție: ne trebuie un contact care e **închis când ușa e deschisă**. Reed-ul obișnuit
   (N.O.) e *închis când magnetul e lângă el*, adică exact invers — telecomanda ar transmite tot
   timpul cât ușa e închisă. Fie luați reed **N.C.**, fie folosiți varianta 3.
3. **EV1527 / PT2262 + SYN115 + reed N.O. + MOSFET P** (circuitul în care reed-ul ține MOSFET-ul
   închis cât ușa e închisă). La 3 V de la CR2032 alegeți **AO3401** (pornește sigur de la ~1 V),
   nu BS250. PT2262 verificați-l în foaia de catalog: unele variante cer minim 4 V.

SYN115 (emițătorul) merge între 1,8 și 3,6 V: CR2032 e bun, o celulă 18650 (4,2 V) l-ar arde.

Codurile nu se scriu în program: pe panou, la Alarmă, atingeți senzorul din listă și în 20 de
secunde deschideți ușa respectivă. Arduino învață codul și îl ține în EEPROM.

## Panoul ESP32 (ESP32-3248S035)

Totul e deja pe placă: ecran ST7796, touch XPT2046, difuzor (IO26), LED RGB (IO4/16/17),
fotorezistență (IO34). Se alimentează prin USB sau prin conectorul P1 (5 V). Folosim conectorul
P3 pentru Arduino. Dacă la voi culorile ies inversate, în [`panou/ecran.h`](../panou/ecran.h)
schimbați `cfg.invert`.
