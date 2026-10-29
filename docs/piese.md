# Piesele

Lista e scrisă așa încât s-o putem pune direct în prezentare („componentele folosite și la ce
folosesc”). Prețurile sunt orientative (sigmanortec, drot, optimus).

Prima parte e ce **avem montat**. A doua parte e ce e **prevăzut în cod**, dar încă nu e legat:
programul îl folosește singur în clipa în care îl legăm (pentru unele trebuie doar decomentată o
linie în `casa/config.h`). Până atunci, Arduino le înlocuiește cu modelul casei.

## Ce avem montat

### Plăcile

| Componentă | Buc. | La ce folosește |
|---|:---:|---|
| Arduino Uno (R3) | 1 | „creierul” casei: citește senzorii, ia toate deciziile, ține minte rutina |
| Placă ESP32 cu ecran tactil 3,5" (ESP32-3248S035) | 1 | panoul de pe perete: ecranul, Wi-Fi, ora, prognoza, aplicația de pe telefon |
| ESP32-S3 | 1 | placa din grădină: senzorul de ploaie și releele electrovalvelor, legată de panou prin ESP-NOW |
| Translator de nivel 4 canale (BSS138) | 1 | legătura I2C dintre Arduino (5 V) și ESP32 (3,3 V) |

### Senzori

| Componentă | Buc. | La ce folosește |
|---|:---:|---|
| DHT11 | 1 | temperatura și umiditatea aerului în dormitor |
| Senzor de ploaie cu modul LM393 (cu potențiometru) | 1 | plouă acum → nu udăm |
| Receptor 433 MHz | 1 | primește codurile de la senzorii de pe uși și geamuri |
| Senzori de ușă / geam wireless 433 MHz (contact magnetic + emițător) | 4 | simt când se deschide ușa sau geamul: alarma și „geam deschis → oprim încălzirea” |

### Ce comandă casa

| Componentă | Buc. | La ce folosește |
|---|:---:|---|
| Emițător 433 MHz FS1000A + antenă de 17,3 cm | 1 | aprinde / stinge întrerupătoarele Livolo, ca o telecomandă |
| Întrerupătoare Livolo (cap de scară) | 2 | lumina din dormitor, comandată de casă |
| Buzzer activ 5 V | 1 | sirena alarmei |
| Modul cu 4 relee 5 V + sursă 5 V / 2 A | 1 | cele 3 electrovalve ale grădinii (al 4-lea releu e liber) |
| Electrovalve | 3 | cele 3 zone de udat din grădină |

## Prevăzute în cod, încă nemontate

| Componentă | Buc. | La ce ar folosi | Până atunci |
|---|:---:|---|---|
| Senzor de mișcare PIR (HC-SR501 sau AM312) | 3 | cine e în ce cameră: luminile se aprind / sting singure, rutina învață pe camere (`ARE_PIR`) | „e cineva acasă” vine de la alarmă |
| Modul cu fotorezistență | 1 | cât de luminos e afară: becurile completează doar ce lipsește (`ARE_LDR`) | lumina de afară estimată după oră și prognoză |
| DS18B20 (unul în capsulă, pentru afară) | 4 | temperatura în fiecare cameră și afară (`ARE_DS18B20`) | modelul termic + prognoza |
| Senzor capacitiv de umiditate a solului | 1 | când are nevoie grădina de apă (`ARE_SENZOR_SOL`) | pământul calculat: soare, udat, ploaie |
| INA219 | 3-4 | cât dă panoul solar, cât dă turbina, cât consumă casa și priza TV (le găsește singur la pornire) | energia calculată |
| Panou solar mic (6 V) + modul de încărcare + acumulator 18650 | 1 | energia de la soare (panoul îl avem deja, încă nelegat) | soarele din prognoză, baterie virtuală |
| Turbină eoliană mică + generator | 1 | energia de la vânt | vânt virtual |
| Mini pompă 5 V + MOSFET | 1 | pompa de irigații (pinul D10) | electrovalvele grădinii |
| Rezistență de încălzire + MOSFET | 1 | „caloriferul” (pinul D11) | doar decizia, în modelul termic |
| Modul releu 5 V | 1 | priza inteligentă a televizorului (pinul D12) | doar decizia |
| LED-uri + rezistențe 220 Ω | 2 | becurile din sufragerie și bucătărie (pinii D5, D9) | becuri virtuale |

## Ce am scos din planul inițial (și de ce)

- **Ecranul OKY4032 fără touch și plăcuța cu 6-7 butoane**: placa ESP32 cu ecran tactil le
  înlocuiește pe amândouă și ne scutește de zeci de fire.
- **Microfonul și asistentul vocal**: frumos, dar nu ajută la energie și ne-ar fi mâncat
  săptămâni. L-am trecut la „dezvoltări ulterioare”.
- **Soneria cu cameră (ESP32-CAM)**: complicată și fără legătură cu tema.
- **Arduino Mega**: am rămas pe Uno, iar codul încape (29,6 KB din 30 KB, și pe clonele cu
  bootloader vechi).
