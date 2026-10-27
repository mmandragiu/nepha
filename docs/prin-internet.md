# Casa prin internet (brokerul MQTT)

Aplicația ajunge la casă pe două drumuri:

```
                  ┌──────────── acasă: Wi-Fi, direct (HTTP) ─────────────┐
                  │                                                      ▼
 TELEFON ─────────┤                                                 PANOU ESP32 ──I2C── ARDUINO
 (aplicația)      │                                                      ▲
                  └── date mobile / alt Wi-Fi: mqtt.mandragiu.eu:8883 ───┘
                                     (broker MQTT, criptat TLS)
```

- **Întâi Wi-Fi-ul.** Aplicația încearcă panoul direct. Dacă răspunde, merge ca până acum: întreabă
  la 2 secunde.
- **Altfel, brokerul.** Dacă panoul nu răspunde în ~3 secunde și aplicația știe casa (vezi
  „Legarea”), trece prin broker. Sus scrie „prin internet”, cu albastru.
- **Revenirea e singură.** La 20 de secunde mai încearcă Wi-Fi-ul. Când ai ajuns acasă, trece înapoi.
- **Arduino nu știe nimic de toate astea.** Pentru el, comenzile vin tot de la panou.

## Cine ce face

| Unde | Fișier | Ce face |
|---|---|---|
| Panou | `panou/mqtt.ino` | se leagă de broker, răspunde la cereri, anunță ce se schimbă |
| Panou | `panou/inrolare.ino` | conturile pe broker, de la serverul de conturi (`nepha-auth.mandragiu.eu`): al panoului și ale telefoanelor |
| Panou | `panou/imperechere.ino` | numele unic al panoului (adresa MAC), cheia casei, codul de pe ecran, desfacerea PIN-ului |
| Panou | `panou/web.ino` | API-ul de pe Wi-Fi; JSON-ul îl fac aceleași funcții pentru ambele drumuri |
| Panou | `panou/config.h` | adresa brokerului și **topicele** (într-un singur loc) |
| Aplicație | `lib/services/broker.dart` | legătura cu brokerul, cererile, criptarea PIN-ului; topicele, la fel ca în panou |
| Aplicație | `lib/providers/app_state.dart` | alege drumul (Wi-Fi sau internet) și trece singură de pe unul pe altul |
| Aplicație | `lib/services/istoric_local.dart` | graficul pe 3 zile, ținut pe telefon (SQLite) |

## Ce circulă pe broker

Trei topice, cum le cere ACL-ul brokerului. `{sid}` e numele panoului (adresa MAC, se vede pe
panou la Setări), `{cid}` e id-ul telefonului (16 litere și cifre, făcut la prima pornire a aplicației).
Ambele sunt și „client ID”-ul cu care se conectează fiecare la broker.

| Topic | Cine scrie | Ce e în el |
|---|---|---|
| `nepha/servers/{sid}/requests` | telefoanele înrolate la casă | o cerere: `{"id","client","server","cere":"stare\|jurnal\|istoric\|rutina\|comanda\|privesc"}` |
| `nepha/servers/{sid}/clients/{cid}/responses` | panoul | răspunsul, doar pentru telefonul care a întrebat: `{"id","ok","date"}` |
| `nepha/servers/{sid}/events/devices` | panoul | noutăți pentru telefoanele casei: `{"tip":"stare\|eveniment\|online\|offline","server","date"}` |

- În `date` e **exact același JSON** ca pe Wi-Fi (`/api/stare`, `/api/jurnal`...).
- Fiecare casă are topicele ei, deci două case pe același broker nu se încurcă.
- Topicele sunt scrise în trei locuri, care trebuie să rămână la fel: `panou/config.h`, `aplicatie/lib/services/broker.dart` și `unelte/proba_broker.py`. Acolo `{server}` = `{sid}` și `{client}` = `{cid}`.

### Cât trafic

- Panoul anunță **doar cât timp cineva are aplicația deschisă**: aplicația îi zice „privesc” o dată pe minut, iar după 2,5 minute fără „privesc”, panoul tace.
- **Ce nu vrem să ratăm** (uși, alarmă, lumini, cine e în ce cameră, pompa, modul) pleacă imediat, cel mult o dată la 2 secunde.
- **Cifrele** (temperaturi, wați) pleacă cel mult o dată la 30 de secunde.
- **Jurnalul și graficul** le cere aplicația când intră pe drumul prin internet. Graficul îl cere apoi la 5 minute, rutina la 10.
- **Pe fundal nu se trimite nimic.** Când aplicația nu e pe ecran, se deconectează de la broker.

### PIN-ul

`ALARMA OPRIT <pin>` nu trece niciodată în clar prin broker:

1. **Legarea, o singură dată, acasă.** Telefonul primește de la panou **cheia casei**, pe Wi-Fi.
2. **Criptarea.** Prin internet, aplicația criptează comanda cu cheia (AES-256-GCM) și pune înăuntru ora trimiterii.
3. **Verificarea pe panou.** Panoul desface comanda și refuză:
   - mesajele mai vechi de 2 minute;
   - pe cele deja văzute (cine prinde mesajul și îl retrimite nu dezarmează nimic);
   - pe cele făcute cu cheia altei case.

   Abia apoi îi dă comanda lui Arduino, care verifică PIN-ul ca de obicei.

## Conturile pe broker

Pe broker, fiecare panou și fiecare telefon are **contul lui** (utilizator și parolă). Conturile le
face serverul de conturi al brokerului (`nepha-auth.mandragiu.eu`):

| Cine | Cerere | Cheia (`X-Enroll-Key`) | Unde ține parola |
|---|---|---|---|
| panoul, singur, la prima pornire | `POST /enroll/server {"serverId"}` | a panourilor | în memoria panoului (Preferences „broker”, nu se șterge când urci alt program) |
| panoul, pentru un telefon, la legare | `POST /enroll/client {"clientId","serverId"}` | a telefoanelor | pe telefon, criptat (`flutter_secure_storage`: Keychain / Keystore) |
| dacă id-ul are deja cont | același POST, cu `"reset": true` | de resetare | se înlocuiește cu parola nouă |

Răspunsul e `{"username", "password"}`.

- **Telefoanele nu se înrolează singure.** Le înrolează panoul, la legare. Așa, cheile de înrolare
  stau doar pe panou (`panou/secrete.h`), nu în aplicație, iar cont primește doar telefonul care a
  văzut codul de pe ecranul panoului.
- **În aplicație nu e nicio parolă.** APK-ul și IPA-ul se pot da oricui, iar repo-ul poate fi public.
- Dacă brokerul refuză parola panoului (contul a fost șters sau resetat), panoul cere singur una nouă.
- `{sid}` = adresa MAC a panoului (12 caractere, de exemplu `a0b1c2d3e4f5`; se vede la Setări).
  `{cid}` = id-ul telefonului (16 litere și cifre, făcut la prima pornire a aplicației). Cu ele se
  conectează fiecare la broker (client ID) și ele apar în topice.

## Legarea telefonului de casă (o dată, acasă)

1. Telefonul în Wi-Fi-ul casei, aplicația conectată la panou (sus scrie „acasă, pe Wi-Fi”).
2. Pe panou: **Setări → Leagă telefonul**. Apare un cod de 6 cifre, valabil 2 minute.
3. În aplicație: **Securitate & Setări → Leagă**, scrii codul.
4. Panoul îi dă telefonului cheia casei și îi face cont pe broker (durează câteva secunde).
5. Gata: scrie „Telefonul e legat de casă: merge și prin internet”.

Un cod e bun pentru un singur telefon; după 5 coduri greșite, panoul îl anulează. Dacă panoul n-are
internet în momentul ăla, telefonul primește doar cheia: merge pe Wi-Fi, iar pentru internet îl legi
din nou mai târziu.

## Când se resetează ceva

| Ce s-a întâmplat | Ce face sistemul singur | Ce faci tu |
|---|---|---|
| Panoul a fost reprogramat (programul nou peste cel vechi) | contul, cheia casei și numele panoului rămân în memoria lui | nimic |
| Memoria panoului a fost ștearsă de tot („Erase all flash”) | se înrolează din nou; are deja cont, deci cere parolă nouă (`reset`). Își face și o cheie a casei nouă | legi din nou telefoanele, ca să primească noua cheie (aplicația îți spune când dezarmarea prin internet nu mai merge) |
| Brokerul nu mai primește parola panoului (cont șters sau resetat) | panoul cere singur parolă nouă (`reset`) | nimic |
| Aplicația a fost reinstalată sau i s-au șters datele | își face un id nou și uită tot din seif (pe iPhone, Keychain-ul rămâne după dezinstalare; îl golim noi) | o legi din nou, acasă |
| Brokerul nu mai primește contul telefonului | aplicația uită contul și îți spune | o legi din nou, acasă |
| Legi din nou același telefon | panoul cere pentru el parolă nouă (`reset`) | nimic |

Telefonul nu își poate reface singur contul: doar panoul are cheile de înrolare, deci e nevoie de
codul de pe ecranul lui. După o reinstalare, contul vechi al telefonului rămâne pe serverul de conturi,
nefolosit. Serverul l-ar putea șterge după o vreme.

## Unde pun cheile (pas cu pas)

Singurul loc cu chei e **panoul**. Ele stau în `panou/secrete.h`, pe care git îl ignoră. În plus,
un control local (`.git/hooks/pre-commit`) oprește orice commit care l-ar conține. **Nu-l pune nici
pe dontpad:** paginile de acolo sunt publice.

1. În folderul `panou/`, fă o copie a lui `secrete.exemplu.h` și numește-o `secrete.h`.
2. În `secrete.h`, în locul textelor de exemplu, scrie cele trei chei de la tatăl tău:
   - `INROLARE_PANOURI`: cheia pentru `/enroll/server`;
   - `INROLARE_TELEFOANE`: cheia pentru `/enroll/client`;
   - `INROLARE_RESET`: cheia de resetare.
3. Urcă programul pe panou, ca de obicei.
4. Ca să verifici: pe panou, la **Setări**, trebuie să scrie „prin internet: da”. În Serial Monitor
   apare „Inrolare /enroll/server: 200” și „Broker: conectat”.

Colegul care urcă programul pe panou are nevoie de același `secrete.h`. Dă-i fișierul direct (stick,
mesaj privat), nu prin GitHub sau dontpad.

Aplicației (Android sau iPhone) nu-i trebuie nimic: nici `secrete.json`, nici GitHub Secrets.
Secretele `MQTT_UTILIZATOR` și `MQTT_PAROLA` din GitHub nu mai sunt folosite și se pot șterge.

### Verificare fără panou

`unelte/proba_broker.py` (trebuie `pip install paho-mqtt cryptography`) ia parolele din aceleași
fișiere și nu le afișează.

- `python unelte/proba_broker.py panou` pornește pe laptop un panou de probă (`nephaproba01`), cu o
  casă inventată (PIN-ul alarmei: 1234). Răspunde:
  - **pe Wi-Fi**, la adresa laptopului, portul 8080 (o afișează la pornire). În aplicație scrii
    `adresa:8080`, de exemplu `192.168.1.10:8080`;
  - **prin broker**, dacă găsește cheile de înrolare în `panou/secrete.h` (își face cont și le face cont telefoanelor legate, ca panoul adevărat).

  Tot el afișează și un cod de legare. Așa probezi tot drumul fără ESP32: Wi-Fi, legarea, apoi
  oprești Wi-Fi-ul telefonului, mergi prin internet și dezarmezi cu PIN-ul criptat.
- `python unelte/proba_broker.py telefon a0b1c2d3e4f5` (numele panoului) se dă drept telefon și întreabă panoul
  adevărat de stare, prin broker.

Dacă brokerul refuză (de exemplu din cauza ACL-ului), scriptul arată codul de eroare.

## Mai departe (după concurs)

- **Cheile de înrolare pe fiecare panou.** Acum toate panourile au aceleași trei chei, deci cine
  scoate cheile din memoria unui panou poate face sau reseta conturi și la alte case. Mai bine ar fi
  ca serverul de conturi să dea fiecărui panou cheia lui, sau ca `/enroll/client` să accepte cereri
  doar de la panoul logat al casei respective.
- **Notificări pe telefon când aplicația e închisă** (alarma sună, ușa s-a deschis): ar trebui
  Firebase (Android) sau APNs (iPhone).
