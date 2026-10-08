# Piesele

Lista e scrisă așa încât s-o putem pune direct în prezentare („componentele folosite și la ce
folosesc”). Prețurile sunt orientative (sigmanortec, drot, optimus).

## Plăcile

| Componentă | Buc. | La ce folosește |
|---|:---:|---|
| Arduino Uno (R3) | 1 | „creierul” casei: citește senzorii, ia toate deciziile, ține minte rutina |
| Placă ESP32 cu ecran tactil 3,5" (ESP32-3248S035) | 1 | panoul de pe perete: ecranul, Wi-Fi, ora, prognoza, pagina pentru telefon |

## Senzori

| Componentă | Buc. | La ce folosește |
|---|:---:|---|
| DS18B20 (unul în capsulă metalică, pentru afară) | 4 | temperatura în fiecare cameră și afară, toți pe un singur fir |
| Senzor de mișcare PIR (HC-SR501 sau AM312) | 3 | cine e în ce cameră; din asta învață casa rutina |
| Modul cu fotorezistență | 1 | cât de luminos e afară: becurile completează doar ce lipsește |
| Senzor capacitiv de umiditate a solului (v1.2) | 1 | când are nevoie grădina de apă (cel capacitiv nu ruginește ca cel cu doi electrozi) |
| INA219 | 3-4 | curent și tensiune: cât dă panoul solar, cât dă turbina, cât consumă casa (și priza TV) |
| Receptor 433 MHz SYN480R | 1 | primește codurile de la senzorii de pe uși și geamuri |
| Emițător 433 MHz SYN115 + EV1527/PT2262 (sau senzori de ușă gata făcuți) | 4 | câte unul pe fiecare ușă / geam |
| Contact magnetic (reed) + magnet | 4 | simte când se deschide ușa sau geamul |

## Ce comandă casa

| Componentă | Buc. | La ce folosește |
|---|:---:|---|
| LED alb cald + rezistență 220 Ω | 3 | becurile din camere (luminozitate reglată prin PWM) |
| Rezistență 25 Ω / 2 W | 1 | „caloriferul” machetei |
| Mini pompă de apă 5 V + furtun | 1 | irigațiile din grădină |
| MOSFET IRLZ44N (sau modul cu MOSFET) + rezistențe 220 Ω / 10 kΩ | 2 | pornesc pompa și rezistența (un pin de Arduino nu le poate alimenta direct) |
| Diodă 1N4007 | 1 | protejează MOSFET-ul pompei |
| Modul releu 5 V | 1 | „priza inteligentă” a televizorului |
| Buzzer activ 5 V | 1 | sirena alarmei |

## Energie

| Componentă | Buc. | La ce folosește |
|---|:---:|---|
| Panou solar mic, 6 V, 1-2 W | 1 | energia de la soare |
| Turbină eoliană cu ax vertical (Savonius, printată 3D) + motor DC ca generator | 1 | energia de la vânt |
| Punte redresoare + modul step-up/step-down | 1 | aduce tensiunea turbinei la 5 V |
| Modul de încărcare CN3791 (pentru panouri solare) sau TP4056 cu protecție | 1 | încarcă bateria |
| Acumulatori 18650 + suport + protecție (BMS) | 1-2 | bateria casei |
| Diode Schottky 1N5819 | 2 | nu lasă bateria să se descarce înapoi în panou / turbină |
| Step-up MT3608 reglat la 5 V | 1 | alimentează consumatorii casei din baterie |

## Legături și mărunțișuri

| Componentă | Buc. | La ce folosește |
|---|:---:|---|
| Translator de nivel 4 canale (BSS138) sau rezistențe 1 kΩ + 2 kΩ | 1 | legătura serială dintre Arduino (5 V) și ESP32 (3,3 V) |
| Rezistență 4,7 kΩ | 1 | firul senzorilor DS18B20 |
| Breadboard / plăcuță de test, fire Dupont, conectori JST | | |
| Condensatori 100 nF și 100 µF | câțiva | lângă modulele care trag curent (pompa, releul) |
| Placaj / carton, lipici, vopsea | | macheta |

## Ce am scos din planul inițial (și de ce)

- **Ecranul OKY4032 fără touch și plăcuța cu 6-7 butoane**: placa ESP32 cu ecran tactil le
  înlocuiește pe amândouă și ne scutește de zeci de fire.
- **Microfonul și asistentul vocal**: frumos, dar nu ajută la energie și ne-ar fi mâncat
  săptămâni. L-am trecut la „dezvoltări ulterioare”.
- **Soneria cu cameră (ESP32-CAM)**: complicată și fără legătură cu tema.
- **Arduino Mega**: am rămas pe Uno, iar codul încape (97% din memorie, cu loc pentru ajustări).
