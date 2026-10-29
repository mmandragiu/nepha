# Cum funcționează codul

Pentru fiecare lucru pe care îl face casa: unde e în cod, cum merge și ce trebuie să se vadă
când îl încercăm. Ne folosim de asta și pentru video-ul cu explicarea codului.

Comenzile din coloana „Cum îl arătăm” se scriu în Serial Monitor (115200, „Newline”), pe
Arduino sau în simularea Wokwi.

---

## Arduino Uno (`casa/`)

### 1. Bucla principală — [`casa.ino`](../casa/casa.ino)

`loop()` face, la fiecare trecere: avansează ceasul, ascultă radioul (uși), citește comenzile
de la panou, citește senzorii. **O dată pe secundă** cheamă `decide()`, care ia deciziile
mereu în aceeași ordine: modelul termic → rutina → ferestrele → energia → încălzirea →
luminile → priza → grădina. Ordinea contează: ca să decidă încălzirea, trebuie să știe deja
dacă e un geam deschis și cât de plină e bateria. La final `aplicaIesiri()` trimite totul pe
pini (luminile se aprind lin, în trepte de 2%).

*Rezultat:* în Serial Monitor apar evenimentele cu ora lor, de exemplu `[07:12] preincalzire 45 8`.

### 2. Ceasul — [`ceas.ino`](../casa/ceas.ino)

Arduino nu are ceas cu baterie: ora vine de la panou (`ORA 1290`). Până atunci pornește de la
07:00. `ceasViteza` permite ca o zi întreagă să treacă în 6 minute (ziua de test).
Rutina învață doar după ce știe ora adevărată.

### 3. Senzorii — [`senzori.ino`](../casa/senzori.ino), [`dht11.h`](../casa/dht11.h), [`ds18b20.h`](../casa/ds18b20.h), [`ina219.h`](../casa/ina219.h)

La pornire vede ce senzori există (`casa.senzori`). Ce lipsește e completat de un model: de
exemplu temperatura unei camere fără senzor o calculează `modelTermic()`. Prezența: un PIR
care a „văzut” mișcare în ultimele 2 minute. Am scris citirea DHT11, DS18B20 și INA219 direct,
în câteva rânduri, pentru că bibliotecile întregi nu mai încăpeau în memoria lui Uno. DHT11 (din
Dormitor) dă și umiditatea aerului. Fără senzor în pământ, umiditatea solului e calculată
(`solPas()` din `gradina.ino`): scade încet peste zi, crește la udat și când plouă.

*Cum îl arătăm:* `SCAN` — arată fiecare termometru și valorile brute ale fotorezistenței și
senzorului de sol (pentru calibrare).

### 4. Rutina (partea care învață) — [`rutina.ino`](../casa/rutina.ino)

Pentru fiecare oră din zi și fiecare cameră, două numere: în câte zile am urmărit ora asta și
în câte dintre ele a fost cineva în cameră (măcar 10 minute). Probabilitatea:

```
p = (ocupat + 1) / (zile + 2)
```

Fără date dă 50% („nu știu”); cu fiecare zi se apropie de realitate. La 30 de zile numerele
se înjumătățesc, ca zilele vechi să conteze mai puțin. Totul stă în EEPROM, deci nu se pierde
când se ia curentul. `minutePanaVineCineva()` caută în orele următoare prima oră cu p ≥ 60%.

*Cum îl arătăm:* `RUTINA` sau ecranul „Rutină” de pe panou (tabelul verde).
*Rezultat:* după ziua de test, orele 7, 17-22 la sufragerie și 0-6 la dormitor sunt verzi.

### 5. Încălzirea — [`termostat.ino`](../casa/termostat.ino)

- **Clasic**: ține confortul (21,5°) zi și noapte.
- **Smart** (`tintaSmart()`): întâi alege temperatura „redusă” (plecați 16°, noapte 19°,
  nimeni 18°), apoi verifică dacă vine cineva curând. Cât de devreme se uită depinde de
  câte grade are de urcat și de **rata învățată** (`rata`, °C pe oră, măsurată de fiecare dată
  când merge caloriferul). Odată pornită, preîncălzirea ține până la ora estimată.
- **Fereastra** (`detecteazaFerestre()`): media temperaturii pe ~5 minute vs. temperatura de
  acum. Diferență de peste 0,5° = geam deschis → pauză 15 minute.
- Histerezis: pornește sub țintă − 0,3°, se oprește peste țintă + 0,2°, ca să nu „țăcăne”.
- `pasGeaman()` — casa geamănă clasică, care trăiește în paralel cu aceeași vreme. Din ea
  știm „cât am economisit azi”.

*Rezultat (ziua de test):* `[04:52] preincalzire 68 6` (vă treziți la 6), `[06:48] fereastra
deschisa 2 1` (bucătăria, dedusă din temperatură), `[08:03] incalzire redusa 0 0` (ați plecat).

### 6. Luminile — [`lumini.ino`](../casa/lumini.ino)

- **Clasic**: aprinzi când intri și e întuneric, rămâne aprinsă până pleacă toată lumea sau
  vă culcați (lumina „uitată”).
- **Smart**: doar unde e cineva; nivelul = cât lipsește din lumina de afară (`NIVEL_DORIT -
  luminaNaturala`). Cât așteaptă după ultima mișcare: între 2,5 și 7,5 minute, mai mult la
  orele când de obicei stați în cameră (din rutină). Noaptea: doar la mișcare, 20%. În modul
  de economie, maximum 60%.

*Cum îl arătăm:* în Wokwi, apăsăm pe senzorul PIR al unei camere cu fotorezistența pe puțină
lumină: LED-ul se aprinde; după câteva minute fără mișcare se stinge și în Serial apare `E 7 ...` (evenimentul 7, „lumina stinsă”; pe panou apare în cuvinte).

### 7. Energia și priza TV — [`energie.ino`](../casa/energie.ino)

- Bateria: din tensiune (INA219), sau, fără senzor, o baterie virtuală care adună producția și
  scade consumul.
- Modul (`decideEnergie()`), pe medii de 15 minute: **critic** sub 12%; **economie** sub 30%
  când nu se produce (sub 45% dacă prognoza zice că vine vreme fără soare); **surplus** peste 80%
  când se produce mai mult decât se consumă.
- Priza TV (`decidePriza()`): dacă TV-ul doar stă în standby și nu e nimeni în sufragerie de
  15 minute, o taie; când intră cineva, o pornește la loc.
- `contorizeaza()` adună energia pe categorii, la scara unei case adevărate (bec 12 W, calorifer
  2000 W...), și pe cea măsurată cu INA219 pe machetă.

### 8. Grădina — [`gradina.ino`](../casa/gradina.ino)

Smart: udă doar sub 35% umiditate, nu udă dacă vine ploaia (≥ 60%) — decât dacă pământul e
sub 15% —, preferă dimineața (6-10) și seara (18-21), sau la prânz dacă e surplus de soare.
Clasic: programatorul de la 12, în fiecare zi.

*Rezultat (ziua de test):* `[00:04] udare amanata 80 16` și 0 litri, față de 80 litri la clasic.

### 9. Alarma și senzorii de uși — [`alarma.ino`](../casa/alarma.ino), [`usi.ino`](../casa/usi.ino)

Stări: oprită → se armează (30 s) → armată „acasă” sau „plecat” → intrare (20 s pentru PIN)
→ sună (3 minute, apoi rămâne armată). Trei PIN-uri greșite blochează un minut. Senzorii de
uși trimit un cod radio cât timp ușa e deschisă; dacă nu-l mai aude 3 secunde, ușa s-a închis.
Codurile se învață cu `INVATA 0` (apoi deschizi ușa) și stau în EEPROM.

*Cum îl arătăm:* `ALARMA ACASA`, deschidem un geam învățat → `alarma 5 2` și sirena;
`ALARMA OPRIT 1234` o oprește.

### 10. Legătura cu panoul — [`panou.ino`](../casa/panou.ino), [`legatura.h`](../casa/legatura.h)

Trimite linia de stare o dată pe secundă și câte o linie pentru fiecare eveniment. Citește
comenzi și de la panou, și din Serial Monitor (aceeași funcție, `executaComanda()`).
Totul e descris în [protocol.md](protocol.md).

Pe fir e I2C: panoul îl întreabă pe Arduino, iar `legatura.h` ține textul într-o coadă de
256 de octeți pe care panoul o golește de câteva zeci de ori pe secundă. Pentru restul
programului, legătura arată ca un port serial (`LEGATURA.print(...)`). Dacă panoul e ocupat
mai mult de 40 ms, Arduino nu-l așteaptă: aruncă linia și merge mai departe, pentru că
alarma și senzorii sunt mai importanți decât un mesaj pe ecran.

### 11. Ziua de test — [`scenariu.ino`](../casa/scenariu.ino)

Întâi casa își învață rutina din 7 zile asemănătoare (orele variază ±20-30 minute de la o zi
la alta). Apoi trăiește aceeași zi o dată clasic, o dată smart, cu aceleași intrări (cine e în
ce cameră, temperatura de afară, lumina, ploaia, geamul deschis la 6:45). Tot ce lipsește din
realitate vine de aici; deciziile sunt exact cele din restul programului.

*Cum îl arătăm:* `TEST RAPID` → tabelul cu economia pe categorii (~19% în total).
`TEST SMART` → ziua trece pe machetă în ~6 minute.

### 12. Memoria — [`memorie.ino`](../casa/memorie.ino)

Setările, PIN-ul, codurile senzorilor și rutina, în EEPROM. Scriem doar ce s-a schimbat
(`EEPROM.update`), ca să nu-l tocim.

### 13. Livolo — [`livolo.ino`](../casa/livolo.ino)

Arduino trimite prin emițătorul de 433 MHz (D8) aceleași semnale ca telecomanda Livolo: 16 biți
„numele” telecomenzii, 7 biți butonul (protocolul e din biblioteca LivoloTx, licență MIT).
Butonul doar comută, iar întrerupătorul nu spune cum e, așa că pentru „stins” trimitem „stinge
tot”, iar pentru „aprins” „stinge tot” + butonul. Se trimite doar când lumina Dormitorului trece
pragul de 30% (sub el, lumina de noapte nu aprinde lustra; o trimitere durează ~1 secundă).

---

## Placa din grădină (`gradina/`)

Un ESP32-S3 cu senzorul de ploaie (DO pe GPIO1) și 4 relee (GPIO 6, 4, 15, 7). Nu ia decizii:
spune la 2 secunde dacă plouă și, la cererea panoului, deschide zonele pe rând, câte
`SECUNDE_ZONA`. Își caută singură panoul pe canalele Wi-Fi 1-13; vorbesc prin ESP-NOW, fără router.

---

## Panoul ESP32 (`panou/`)

| Tab | Ce face |
|---|---|
| [`legatura.ino`](../panou/legatura.ino) | desface liniile de la Arduino în structura `casa`; trimite ora (la minut), prognoza (la 10 min) și cere rutina |
| [`texte.ino`](../panou/texte.ino) | transformă codurile evenimentelor în propoziții („Nu ud grădina: vine ploaia...”) |
| [`vreme.ino`](../panou/vreme.ino) | ia prognoza de la open-meteo.com; **învață factorul panoului solar** (producția reală / ce ar fi trebuit după prognoză) și estimează producția din următoarele 6 ore |
| [`retea.ino`](../panou/retea.ino) | Wi-Fi; dacă nu merge, rețeaua proprie `NEPHA-casa`; ora de pe internet |
| [`web.ino`](../panou/web.ino) | pagina casei și API-ul ei (`/api/stare`, `/api/jurnal`, `/api/istoric`, `/api/rutina`, `/api/comanda`) |
| [`istoric.ino`](../panou/istoric.ino) | câte un punct la 5 minute, pentru grafic |
| [`ui.ino`](../panou/ui.ino) + `ui_*.ino` | ecranele; fiecare are `desen...()` și `atinge...()`; se redesenează doar cardurile care s-au schimbat. Animațiile (o stare nouă apare în 200 ms, bara bateriei alunecă în 0,5 s, punctul din „Smart” pulsează) sunt cadre scurte, desenate la ~30 ms, așa că nu opresc citirea mesajelor de la Arduino |
| [`sunet.ino`](../panou/sunet.ino) | bipuri, sirena, LED-ul RGB |
| [`gradina.ino`](../panou/gradina.ino) | ESP-NOW cu placa din grădină: află dacă plouă (îi spune lui Arduino `PLOAIE 1/0`) și, când Arduino pornește pompa, îi cere grădinii să ude zonele pe rând |

Fonturile cu diacritice sunt făcute din Nunito cu [`unelte/fonturi_vlw.py`](../unelte/fonturi_vlw.py).
Pagina web e în [`web/index.html`](../web/index.html); după ce o modificați rulați
`node unelte/web_in_panou.mjs` ca s-o puneți în panou.

---

## Un plan pentru video-ul cu codul (5-7 minute)

1. Arhitectura: cine decide (Arduino) și cine arată (panoul). `casa.ino`: `loop()` și `decide()`.
2. Rutina: formula, tabelul de pe panou, de ce Laplace.
3. Termostatul: `tintaSmart()` + preîncălzirea + fereastra dedusă din temperatură.
4. Luminile și priza TV: clasic vs. smart, în câteva rânduri fiecare.
5. Energia și grădina: modurile de energie, ploaia din prognoză.
6. `TEST RAPID` în Serial Monitor, cu tabelul de rezultate.
7. Panoul: cum devine `E 11 80 16` propoziția din jurnal; pagina web.
