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
| Panou | `panou/imperechere.ino` | numele unic al panoului (`nepha-xxxxxx`), cheia casei, codul de pe ecran, desfacerea PIN-ului |
| Panou | `panou/web.ino` | API-ul de pe Wi-Fi; JSON-ul îl fac aceleași funcții pentru ambele drumuri |
| Panou | `panou/config.h` | adresa brokerului și **topicele** (într-un singur loc) |
| Aplicație | `lib/services/broker.dart` | legătura cu brokerul, cererile, criptarea PIN-ului; topicele, la fel ca în panou |
| Aplicație | `lib/providers/app_state.dart` | alege drumul (Wi-Fi sau internet) și trece singură de pe unul pe altul |
| Aplicație | `lib/services/istoric_local.dart` | graficul pe 3 zile, ținut pe telefon (SQLite) |

## Ce circulă pe broker

Trei topice, cum le cere ACL-ul brokerului. `{sid}` e numele panoului (`nepha-xxxxxx`, se vede pe
panou la Setări), `{cid}` e id-ul telefonului (`nepha-app-...`, făcut la prima pornire a aplicației).
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

## Legarea telefonului de casă (o dată, acasă)

1. Telefonul în Wi-Fi-ul casei, aplicația conectată la panou (sus scrie „acasă, pe Wi-Fi”).
2. Pe panou: **Setări → Leagă telefonul**. Apare un cod de 6 cifre, valabil 2 minute.
3. În aplicație: **Securitate & Setări → Leagă**, scrii codul.
4. Gata: scrie „Telefonul e legat de casă”. De acum merge și de pe date mobile.

Fără legare, prin internet merge tot în afară de dezarmare. Un cod e bun pentru un singur telefon;
după 5 coduri greșite, panoul îl anulează.

## Unde pun parolele (pas cu pas)

Parolele **nu** se scriu în cod și **nu** ajung pe GitHub. Stau în fișiere pe care git le ignoră
(`.gitignore`). În plus, un control local (`.git/hooks/pre-commit`) oprește orice commit care le-ar
conține. **Nu le pune nici pe dontpad:** paginile de acolo sunt publice.

### Panoul (parola lui `nepha-server`)

1. În folderul `panou/`, fă o copie a lui `secrete.exemplu.h` și numește-o `secrete.h`.
2. Deschide `secrete.h` și scrie parola adevărată în locul textului `aici parola lui nepha-server`.
3. Urcă programul pe panou, ca de obicei.
4. Ca să verifici: pe panou, la **Setări**, trebuie să scrie „prin internet: da”.

Colegul care urcă programul pe panou are nevoie de același `secrete.h`. Dă-i fișierul direct (stick,
mesaj privat), nu prin GitHub sau dontpad.

### Aplicația pe Android (parola lui `nepha-client`)

1. În folderul `aplicatie/`, fă o copie a lui `secrete.exemplu.json` și numește-o `secrete.json`.
2. Scrie parola în locul textului `aici parola lui nepha-client`.
3. Fă APK-ul cu:
   ```
   cd aplicatie
   flutter build apk --release --dart-define-from-file=secrete.json
   ```
   Fără `--dart-define-from-file`, APK-ul iese fără broker și merge doar pe Wi-Fi.

### Aplicația pe iPhone (GitHub Secrets)

Aplicația de iPhone se compilează pe Mac-ul de la GitHub, așa că parola trebuie să fie în seiful
repo-ului:

1. Pe GitHub, în repo: **Settings → Secrets and variables → Actions → New repository secret**.
2. Nume: `MQTT_UTILIZATOR`, valoare: `nepha-client`. Salvezi.
3. Încă unul. Nume: `MQTT_PAROLA`, valoare: parola lui `nepha-client`. Salvezi.
4. **Actions → Aplicatia → Run workflow** (cu „iPhone” bifat). În `.ipa` ajunge și parola.

GitHub nu-ți mai arată parola niciodată: doar că există. În loguri apare ca `***`.

**Dacă faci repo-ul public:**
1. pe GitHub, la **Actions**, șterge rulările vechi ale lui „Aplicatia” (cu ele se șterg și fișierele `.ipa`);
2. schimbați parola lui `nepha-client` pe broker și în Secret.

### Verificare fără panou și fără telefon

`unelte/proba_broker.py` (trebuie `pip install paho-mqtt`) ia parolele din aceleași fișiere și nu
le afișează:

- `python unelte/proba_broker.py panou` se dă drept un panou de probă (`nepha-proba`), cu o casă
  inventată. Așa poți încerca aplicația prin internet fără ESP32.
- `python unelte/proba_broker.py telefon nepha-xxxxxx` se dă drept telefon și întreabă panoul
  adevărat de stare.

Dacă brokerul refuză (de exemplu din ACL), scriptul arată codul de eroare.

## Mai departe (după concurs)

- **Un cont pe fiecare telefon.** Un server mic creează un cont pe broker pentru fiecare telefon,
  doar după ce telefonul arată codul de pe panou (cum e acum legarea). Atunci parola nu mai stă în
  aplicație, iar fiecare telefon vede doar casa lui.
- **Notificări pe telefon când aplicația e închisă** (alarma sună, ușa s-a deschis): ar trebui
  Firebase (Android) sau APNs (iPhone).
