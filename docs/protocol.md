# Ce își spun Arduino și panoul

Mesajele sunt text simplu: o linie = un mesaj. Am ales text (nu bytes) ca să putem citi totul
în Serial Monitor și să scriem comenzile de mână când testăm.

Pe fir, textul merge pe **I2C** (prin translatorul de nivel). Panoul e „șeful”, Arduino e la
adresa 8:

- de vreo 40 de ori pe secundă panoul cere 32 de octeți: Arduino îi dă ce are de trimis, iar
  unde nu mai are nimic pune `0` (dacă tocmai citea un INA219, nu dă nimic și panoul vede `0xFF`);
- comenzile panoului pleacă în bucăți de cel mult 32 de octeți (atât încape odată în Arduino).

Așa, de o parte și de alta, programul citește și scrie linii ca pe un port serial
([`casa/legatura.h`](../casa/legatura.h), [`panou/legatura.ino`](../panou/legatura.ino)).

## De la Arduino spre panou

### `S` — starea casei (o dată pe secundă)

```
S h=1132 m=1 sc=0 t=21.6,19.4,21.1 te=7.5 ln=12 p=5 l=80,0,45 f=2 i=1 ti=21.5 mi=2 rt=3.4
  b=78 ps=0.12 pv=0.34 pc=1.28 e=0 a=0 u=4 sol=31 pm=0 pr=1 wh=5230,7480 mwh=0 ok=159
```

| Câmp | Ce înseamnă |
|---|---|
| `h` | ora casei, în minute de la miezul nopții (1132 = 18:52) |
| `m` | modul: 0 clasic, 1 smart |
| `sc` | ziua de test: 0 nu, 1 rulează pe machetă, 2 test rapid |
| `t` | temperaturile camerelor (`-` = nu știm) |
| `te` | temperatura de afară |
| `ln` | lumina naturală, 0-100 |
| `p` | prezența: bitul *i* = e cineva în camera *i* |
| `l` | cât de aprins e becul în fiecare cameră, 0-100 |
| `f` | ferestre deschise (bit pe cameră; de la senzor sau deduse) |
| `i`, `ti` | merge încălzirea? spre ce temperatură |
| `mi` | de ce: 0 clasic, 1 e cineva, 2 preîncălzire, 3 plecați, 4 nu e nimeni, 5 noapte, 6 fereastră |
| `rt` | cu câte grade pe oră se încălzește casa (învățat) |
| `b` | bateria, % |
| `ps`, `pv`, `pc` | producția solară, a turbinei și consumul, în W (la scara machetei) |
| `e` | modul de energie: 0 normal, 1 surplus, 2 economie, 3 critic |
| `a` | alarma: 0 oprită, 1 se armează, 2 acasă (noapte), 3 plecat, 4 intrare (PIN!), 5 sună |
| `u` | senzorii de uși/geamuri deschiși (bit pe senzor) |
| `sol`, `pm` | umiditatea pământului (%), merge pompa |
| `pr` | priza TV pornită |
| `wh` | energia de azi, în Wh pentru o casă adevărată: NEPHA, casa clasică „geamănă” |
| `mwh` | energia măsurată cu INA219 pe machetă, în mWh |
| `ok` | ce senzori a găsit (biți: 1 temp. camere, 2 temp. afară, 4 solar, 8 vânt, 16 consum, 32 priză, 128 oră de la panou) |

### `E` — s-a întâmplat ceva (jurnalul casei)

`E <cod> <a> <b>`. Textele le face panoul ([`panou/texte.ino`](../panou/texte.ino)).

| Cod | Ce | `a` | `b` |
|---:|---|---|---|
| 1 | casa a pornit | modul | |
| 2 | s-a schimbat modul | modul nou | |
| 3 | preîncălzire | în câte minute vine cineva | la ce oră |
| 4 | încălzire redusă | 0 plecați, 1 nu e nimeni, 2 noapte | |
| 5 | fereastră deschisă | camera | 1 dedusă din temperatură, 2 de la senzor |
| 6 | fereastra s-a închis | camera | |
| 7 | lumină stinsă (camera era goală) | camera | |
| 8 | priza TV oprită | minute de standby | |
| 9 | priza TV pornită la loc | | |
| 10 | udare | motivul: 1 surplus, 2 oră bună, 3 pământ foarte uscat, 4 programator, 5 manual | umiditatea |
| 11 | udare amânată, vine ploaia | șansa de ploaie | ora |
| 12 | modul de energie s-a schimbat | modul | bateria % |
| 13 | alarma | starea nouă | senzorul (0-3 uși/geamuri, 100+cameră = mișcare, 255 = nimic) |
| 14 | ușă / geam | senzorul | 1 deschis, 0 închis |
| 15 | casa pare goală, armăm? | | |
| 16 | PIN greșit | încercări rămase | |
| 17 | senzor învățat | slotul | 1 da, 0 n-a auzit nimic |
| 18 | ziua de test | 1 pornită, 0 oprită, 2 gata | modul |

### `C` — rezultatul zilei de test

`C <mod> <lumini> <încălzire> <udare> <priză> <total> <mWh măsurați> <litri>` (Wh pentru o casă adevărată).

### `R` și `Z` — rutina

`R <camera> p0,p1,...,p23` — probabilitatea (%) să fie cineva în cameră la fiecare oră.
`Z z0,...,z23` — din câte zile e învățată fiecare oră.

### `#` — mesaje pentru oameni

Linii care încep cu `#` (de exemplu tabelul de la `TEST RAPID`). Panoul doar le afișează pe Serial.

## De la panou (sau din Serial Monitor) spre Arduino

| Comanda | Ce face |
|---|---|
| `ORA <minut>` | ora de pe internet (panoul o trimite la fiecare minut) |
| `VREME <ploaie%> <ora ploii> <soare%> <temp x10>` | prognoza, la 10 minute; ora 255 = nu plouă |
| `MOD SMART` / `MOD CLASIC` | cum se poartă casa |
| `ALARMA ACASA` / `ALARMA PLECAT` | armare (pe noapte / plecăm) |
| `ALARMA OPRIT <pin>` | dezarmare; PIN-ul îl verifică Arduino (implicit 1234) |
| `PIN <vechi> <nou>` | schimbă PIN-ul |
| `TEMP CONFORT\|NOAPTE\|ECO\|PLECAT <grade x10>` | temperaturile (ex. `TEMP CONFORT 215`) |
| `LUMINA <camera> <0-100>` / `LUMINA <camera> AUTO` | aprinde/stinge de mână (o oră), sau înapoi pe automat |
| `PRIZA 1` / `PRIZA 0` / `PRIZA AUTO` | priza TV |
| `UDA` | pornește pompa acum |
| `INVATA <senzor>` / `STERGE <senzor>` | învață / uită codul radio al unui senzor de ușă/geam |
| `USA <senzor> <0\|1>` | „ușa e de fapt închisă / deschisă”: o punem de mână (nu pornește alarma); rămâne așa până transmite senzorul din nou |
| `TEST RAPID` | ziua de test, calculată pe loc, cu tabelul de rezultate |
| `TEST CLASIC` / `TEST SMART` / `TEST STOP` | ziua de test pe machetă (~6 minute) |
| `RUTINA` / `RUTINA STERGE` | trimite rutina / o uită |
| `SCAN` | ce senzori vede Arduino și ce citește (pentru montaj și calibrare) |
| `STARE` | linia `S` de acum, pe Serial |
| `AJUTOR` | lista scurtă de comenzi |
