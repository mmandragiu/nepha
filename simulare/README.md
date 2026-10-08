# Simularea în Wokwi

Programul pentru Arduino Uno merge și fără componente, în simulatorul [Wokwi](https://wokwi.com).
LED-urile țin locul becurilor, al încălzirii, al pompei și al prizei TV; senzorii PIR, fotorezistența
și un potențiometru (umiditatea solului) se pot mișca cu mouse-ul. Ce nu există în Wokwi
(termometrele, INA219, radioul de la uși) e înlocuit automat de modelul casei din program.

## Cum facem proiectul

1. Pe wokwi.com: *New Project* → *Arduino Uno*.
2. În `sketch.ino` punem conținutul lui [`casa/casa.ino`](../casa/casa.ino).
3. Cu „+” adăugăm celelalte fișiere din `casa/`, cu aceleași nume: `config.h`, `stare.h`,
   `ds18b20.h`, `ina219.h` și toate `.ino`-urile (`alarma.ino`, `ceas.ino`, `energie.ino`, ...).
4. Înlocuim `diagram.json` cu [cel de aici](diagram.json).
5. Adăugăm `libraries.txt` cu [conținutul de aici](libraries.txt).
6. *Start*. Salvăm proiectul și trimitem link-ul (merge deschis de oricine, fără cont).

## Ce încercăm

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
