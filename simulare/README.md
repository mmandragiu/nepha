# Simulare

Avem două feluri de a încerca proiectul fără componente.

## 1. Casa virtuală (în browser) — tot proiectul

[`casa-virtuala.html`](casa-virtuala.html): dublu-click și se deschide în browser, fără internet
și fără instalări.

Rulează **codul adevărat** al ambelor plăci: programul lui Arduino Uno (`casa/`) și programul
panoului (`panou/`), aceleași fișiere ca pe plăci, compilate pentru browser. Cele două
programe își trimit aceleași mesaje ca pe fir (`S ...`, `E ...`, `ORA ...`).

- **Ecranul panoului** se atinge cu mouse-ul, ca pe placă. Are aceleași ecrane, aceleași fonturi
  și aceleași butoane. După un minut fără atingere se întunecă; prima atingere doar îl aprinde,
  exact ca pe panoul adevărat.
- **Macheta** desenată arată ce face Arduino: becurile, caloriferul, pompa, televizorul,
  sirena, geamurile și ușa.
- **Senzorii** sunt butoane: cine e în ce cameră, ușile și geamurile, lumina de afară,
  umiditatea solului și prognoza (temperatura, dacă vine ploaia).
- **Serial Monitor**: ce scrie Arduino și o casetă pentru comenzi (`TEST RAPID`, `TEST SMART`,
  `ALARMA PLECAT`...). Dacă bifați opțiunea de sub el, vedeți și mesajele dintre plăci.

Câteva lucruri de încercat:

| Faceți | Ce se întâmplă |
|---|---|
| Lumina de afară la minim, apoi „Sufragerie” la „Cine e acasă” | becul din sufragerie se aprinde, se stinge la câteva minute după ce plecați |
| `ALARMA ACASA` (sau din panou: Alarmă → Noapte), apoi „Geam dormitor” | sirena, iar panoul trece singur pe ecranul de alarmă; Dezarmează → PIN `1234` |
| „Vine ploaia” cu pământul uscat (sub 35%) | dimineața sau seara casa **nu** udă și scrie de ce, în jurnal |
| `TEST RAPID` | tabelul cu ziua de test: clasic vs. smart |
| `TEST SMART` | aceeași zi, pe machetă: o oră trece în 15 secunde |

Ce nu există în simulator: termometrele DS18B20 și modulele INA219. Programul le înlocuiește
singur cu modelul casei, la fel ca pe o machetă fără ele. Ora vine de la calculator, iar
prognoza o alegeți voi din glisoare.

### Cum se construiește din nou

Doar dacă modificați codul. Aveți nevoie de [Emscripten](https://emscripten.org/docs/getting_started/downloads.html)
(am folosit versiunea 3.1.74), activat în terminal, apoi:

```
node simulare/browser/construieste.mjs
```

Scriptul face o copie a panoului în care schimbă doar fișierul `ecran.h`: în loc de afișajul
ST7796, desenează într-o imagine pe care pagina o pune pe ecran
([`browser/ecran.h`](browser/ecran.h)). Restul codului panoului și al lui Arduino rămâne neatins.
[`browser/Arduino.h`](browser/Arduino.h) imită mediul Arduino, iar [`browser/sim.cpp`](browser/sim.cpp)
leagă cele două programe între ele și de pagină.

## 2. Wokwi — doar Arduino Uno

Programul lui Arduino merge și în simulatorul [Wokwi](https://wokwi.com), cu LED-uri, senzori
PIR, fotorezistență și un potențiometru pentru umiditatea solului.

1. Pe wokwi.com: *New Project* → *Arduino Uno*.
2. În `sketch.ino` punem conținutul lui [`casa/casa.ino`](../casa/casa.ino).
3. Cu „+” adăugăm celelalte fișiere din `casa/`, cu aceleași nume: `config.h`, `stare.h`,
   `ds18b20.h`, `ina219.h` și toate `.ino`-urile (`alarma.ino`, `ceas.ino`, `energie.ino`, ...).
4. Înlocuim `diagram.json` cu [cel de aici](diagram.json).
5. Adăugăm `libraries.txt` cu [conținutul de aici](libraries.txt).
6. *Start*. Salvăm proiectul și trimitem link-ul (se deschide de oricine, fără cont).

În Serial Monitor (jos):

| Scriem | Ce se întâmplă |
|---|---|
| `TEST RAPID` | casa trăiește aceeași zi clasic și smart și scrie tabelul cu economia |
| `TEST SMART` | aceeași zi, dar pe LED-uri: o oră trece în 15 secunde (se oprește singură la final) |
| `ORA 1110` | e 18:30 |
| `STARE` | starea de acum (temperaturi, lumini, baterie...) |
| `ALARMA PLECAT` | după 30 s alarma e armată; apăsați un senzor PIR → 20 s pentru PIN → sirena |
| `ALARMA OPRIT 1234` | dezarmare |
| `MOD CLASIC` / `MOD SMART` | cum se poartă casa |
| `RUTINA` | ce a învățat (după o zi de test, rutina se resetează la cea din EEPROM) |

Cu fotorezistența pe puțină lumină (sub ~20 lux) și un PIR apăsat, becul camerei se aprinde;
după câteva minute fără mișcare se stinge singur și în Serial apare `lumina stinsa`.
Potențiometrul de sol sub 35% + `UDA` sau o oră bună de udat → LED-ul albastru (pompa).
