# Montajul

Tot ce ține de pini e și în [`casa/config.h`](../casa/config.h) și [`panou/config.h`](../panou/config.h).
Dacă mutați un fir, schimbați acolo și atât.

## Arduino Uno — „creierul”

| Pin | Ce e legat | Observații |
|---|---|---|
| D2 | receptor 433 MHz SYN480R (DATA) | trebuie D2: biblioteca RCSwitch folosește întreruperea 0 |
| D3 | DHT11 (DATA): temperatura și umiditatea din Dormitor | modulul are deja rezistența; VCC 5 V, GND |
| D4 | (opțional) firul comun al senzorilor DS18B20 | rezistență 4,7 kΩ între D4 și 5 V; `ARE_DS18B20` în `config.h` |
| D5 | (opțional) bec Sufragerie | PWM |
| D6 | (opțional) bec Dormitor | PWM; lumina reală din dormitor e Livolo |
| D7 | (opțional) PIR Sufragerie | `ARE_PIR` în `config.h` |
| D8 | emițător 433 MHz FS1000A (DATA), pentru Livolo | VCC 5 V, GND; antenă: un fir drept de 17,3 cm |
| D9 | (opțional) bec (LED) Bucătărie | PWM, prin rezistență 220 Ω |
| D10 | MOSFET pompă | |
| D11 | MOSFET încălzire (rezistența) | |
| D12 | releu priza TV | releele chinezești se activează pe LOW (`RELEU_PORNIT`) |
| D13 | buzzer activ (sirena) | prin tranzistor sau modul de buzzer |
| A0 | (opțional) senzor capacitiv umiditate sol (AOUT) | `ARE_SENZOR_SOL` în `config.h`; fără el, umiditatea e calculată |
| A1 | (opțional) fotorezistența (modul, ieșirea AO) | `ARE_LDR`; fără ea, lumina de afară e estimată după oră și prognoză |
| A2 | (opțional) PIR Dormitor | |
| A3 | (opțional) PIR Bucătărie | |
| A4 / A5 | SDA / SCL: panoul (prin translator) și modulele INA219 | toate pe aceleași două fire |
| D0 / D1 | USB | rămân libere, ca să putem urca programul și citi Serial Monitor fără să scoatem fire |

Ce avem acum (29 octombrie): DHT11, Livolo, receptorul pentru uși, buzzer-ul și placa din grădină.
Fără senzori de mișcare, casa știe dacă e cineva acasă după alarmă (armată „Plecat” = nu e nimeni):
luminile le aprindeți voi, iar NEPHA le stinge când plecați. Senzorii de mișcare (luminile care se
aprind singure când intri) sunt următoarea îmbunătățire: se leagă pe D7, A2, A3 și se decomentează
`ARE_PIR`.

Nu mai e niciun pin digital liber. Pentru încă un senzor, cel mai simplu e unul pe I2C (de
exemplu un senzor de lumină BH1750): se leagă tot pe A4 / A5.

## Livolo — întrerupătoarele din dormitor

Arduino se preface că e telecomanda Livolo, prin emițătorul de pe D8 (receptorul de pe D2 doar
ascultă, nu poate trimite). Lumina Dormitorului din program e și lumina Livolo: de la 30% în sus
înseamnă aprins; mai jos (lumina de noapte, când te miști noaptea) Livolo rămâne stins. Legarea, o singură dată, pe rând la fiecare întrerupător (la cap de scară, la
amândouă):
1. țineți degetul pe întrerupător până piuie (~5 secunde);
2. în aplicație: Camere → Dormitor → **Leagă întrerupătorul Livolo** (sau `LIVOLO LEAGA` în
   Serial Monitor), în câteva secunde.

Livolo nu spune înapoi dacă e aprins. Dacă cineva apasă întrerupătorul de mână, casa nu află; la
următoarea schimbare trimite iar „stinge tot” (+ „aprinde”) și se potrivesc din nou.

## Placa din grădină (ESP32-S3)

| GPIO | Ce e legat | Observații |
|---|---|---|
| 1 | senzorul de ploaie, DO (plăcuța VAR34 + modulul cu LM393) | VCC 3,3 V; pragul din potențiometru (LED-ul de pe modul = ud) |
| 2 | senzorul de ploaie, AO | doar pentru calibrare (apare pe Serial) |
| 6, 4, 15 | IN1, IN2, IN3 pe modulul cu relee (zonele 1-3; IN4 rămâne liber) | releele se activează pe LOW; VCC 5 V dintr-o sursă separată de 1,5-2 A, GND comun |

Placa în Arduino IDE: **ESP32S3 Dev Module**. Nu are nevoie de biblioteci și nici de Wi-Fi: își
găsește singură panoul, prin ESP-NOW. Cât se udă fiecare zonă: `SECUNDE_ZONA` în program.

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
Dacă vreți ca temperatura să crească de-adevăratelea pe senzor, puneți senzorul de temperatură
aproape de rezistență. Camerele fără senzor folosesc oricum modelul termic din program.

## Senzorii de temperatură DS18B20 (opțional)

Programul îi citește doar cu `#define ARE_DS18B20` în `casa/config.h` (ocupă 1,5 KB, iar programul
trece atunci de 30 KB: merge pe Uno original, nu pe clonele cu bootloader vechi). Toți pe același fir (D4), fiecare cu VCC la 5 V și GND. O singură rezistență de 4,7 kΩ între
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

## Legătura Arduino ↔ panou (I2C, prin translatorul de nivel)

Arduino lucrează la 5 V, panoul la 3,3 V, așa că firele trec prin translatorul de nivel cu
4 canale (BSS138). Pe el scrie „I2C”, dar e doar un exemplu de folosire: fiecare canal duce
un semnal în ambele sensuri.

| Panou (conectorul cu IO21 / IO22) | Translator | Arduino |
|---|---|---|
| 3,3 V | LV | |
| 5 V | HV | 5 V |
| GND | GND | GND (masa comună, obligatoriu) |
| IO21 (SDA) | LV1 ↔ HV1 | A4 (SDA) |
| IO22 (SCL) | LV2 ↔ HV2 | A5 (SCL) |

Panoul e „șeful” pe fir: de vreo 40 de ori pe secundă îl întreabă pe Arduino ce are de spus și
îi trimite comenzile. Arduino răspunde la adresa **8**. Modulele INA219 stau tot pe A4 / A5,
de partea lui Arduino (HV), la adresele lor (0x40, 0x41, 0x44, 0x45).

Pe Uno nu folosim pinii 0/1, așa că putem urca programul și vedea Serial Monitor fără să
deconectăm panoul.

Alimentarea: tot ansamblul pornește din USB-C-ul panoului (un încărcător de telefon de 2 A).
Din 5 V-ul panoului se alimentează Arduino (pinul 5 V) și partea HV a translatorului.

## Senzorii wireless de pe uși și geamuri (433 MHz)

Receptorul **SYN480R** stă lângă Arduino (VCC 5 V, GND, DATA la D2). Pe receptor și pe fiecare
emițător, câte o antenă: un fir de 17,3 cm.

Pe fiecare ușă / geam, un emițător care trimite un cod unic cât timp ușa e deschisă.

**La noi:** câte o telecomandă cu 4 canale (433 MHz, EV1527) pentru fiecare ușă, cu contactul
magnetic MC-38 și un tranzistor lipite pe butonul din stânga-sus. Cât ușa e deschisă,
telecomanda transmite; când se închide, tace. Arduino socotește ușa închisă după 2 secunde fără
semnal (`TIMP_INCHIDERE_MS`). Telecomanda se mai oprește singură după o vreme chiar dacă ușa a
rămas deschisă: atunci ușa apare închisă, iar pe panou (sau pe telefon) atingeți starea ei ca să
o puneți cum e de fapt. Starea pusă de mână rămâne așa până când senzorul transmite din nou.

Alte variante, de la cea mai simplă:

1. **Senzori de ușă gata făcuți, 433 MHz** (15-30 lei). Merg direct.
2. **Telecomandă 433 MHz cu EV1527** (de tip breloc), desfăcută, cu contactul magnetic lipit pe
   un buton. Atenție: ne trebuie un contact care e **închis când ușa e deschisă**. Reed-ul obișnuit
   (N.O.) e *închis când magnetul e lângă el*, adică exact invers — telecomanda ar transmite tot
   timpul cât ușa e închisă. Fie luați reed **N.C.**, fie folosiți varianta 3.
3. **EV1527 / PT2262 + SYN115 + reed N.O. + MOSFET P** (circuitul în care reed-ul ține MOSFET-ul
   închis cât ușa e închisă). La 3 V de la CR2032 alegeți **AO3401** (pornește sigur de la ~1 V),
   nu BS250. PT2262 verificați-l în foaia de catalog: unele variante cer minim 4 V.

SYN115 (emițătorul) merge între 1,8 și 3,6 V: CR2032 e bun, o celulă 18650 (4,2 V) l-ar arde.

Codurile nu se scriu în program: pe panou, la Alarmă, atingeți numele senzorului din listă și în
20 de secunde deschideți ușa respectivă. Arduino învață codul și îl ține în EEPROM. (Atingerea
pe starea din dreapta, „deschis” / „închis”, o schimbă de mână.)

## Panoul ESP32 (ESP32-3248S035)

Totul e deja pe placă: ecran ST7796, touch XPT2046, difuzor (IO26), LED RGB (IO4/16/17),
fotorezistență (IO34). Se alimentează prin USB-C sau prin conectorul P1 (5 V). Pentru Arduino
folosim conectorul cu IO21 / IO22 (vezi mai sus). Dacă la voi culorile ies inversate, în [`panou/ecran.h`](../panou/ecran.h)
schimbați `cfg.invert`.
