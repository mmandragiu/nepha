# NEPHA — aplicația de telefon (Flutter)

Aceeași casă ca pe panou și pe pagina casei, dar ca aplicație pentru Android și iPhone. Aplicația nu
ia nicio decizie: doar arată ce face casa și trimite comenzile noastre, prin panoul ESP32. Creierul
rămâne Arduino.

<p>
  <img src="../docs/imagini/aplicatie/acasa.png" width="200" alt="Acasă">
  <img src="../docs/imagini/aplicatie/camere.png" width="200" alt="Camere și grădină">
  <img src="../docs/imagini/aplicatie/energie.png" width="200" alt="Energie">
  <img src="../docs/imagini/aplicatie/securitate.png" width="200" alt="Securitate și setări">
</p>

Patru file:

- **Acasă** — economia de azi (kWh și lei, față de o casă clasică), în casă / afară, bateria,
  alarma și jurnalul deciziilor casei;
- **Camere & Grădină** — luminile (se aprind de aici), umiditatea solului cu „Udă acum” și ce a
  învățat casa (pe camere și pe ore);
- **Energie** — soarele, vântul, bateria și consumul, graficul zilei și ziua de test (clasic vs smart);
- **Securitate & Setări** — alarma (PIN, Face ID / Touch ID sau amprentă), ușile și geamurile, adresa panoului, modul
  demo, Smart / Clasic, temperatura de confort și tema (ca telefonul, zi sau noapte).

## Cum o pornim

Avem nevoie de [Flutter](https://docs.flutter.dev/get-started/install) și, pentru Android, de
**Android Studio** (aduce Android SDK-ul). Telefonul se leagă cu cablu USB, cu *USB debugging* pornit.

```
cd aplicatie
flutter pub get
flutter run
```

Fără telefon, se poate încerca și pe calculator: `flutter run -d windows` (merge și cu panoul
adevărat) sau `flutter run -d chrome` (în browser doar modul demo: panoul nu permite cereri din
pagini străine).

## Cum o legăm de casă

1. Telefonul în aceeași rețea Wi-Fi cu panoul (sau în rețeaua panoului, `NEPHA-casa`, parola `casa2026`).
2. Pe panou, la **Setări**, scrie adresa lui (de exemplu `192.168.1.42`; în rețeaua `NEPHA-casa` e
   `192.168.4.1`).
3. În aplicație, la **Securitate & Setări**: scriem adresa, apăsăm **Conectează**.

Sus, în dreapta, se vede legătura: verde = acasă, pe Wi-Fi, albastru = prin internet, galben = se
conectează sau Arduino nu răspunde, roșu = panoul nu răspunde. Fără panou, **modul demo** arată o seară simulată (e pornit la prima
deschidere), iar comenzile merg și acolo.

## Prin internet

Când telefonul nu e în Wi-Fi-ul casei (date mobile, alt Wi-Fi), aplicația merge prin brokerul MQTT
`mqtt.mandragiu.eu`. Trece singură de pe un drum pe altul. Cum funcționează, legarea telefonului de
casă și unde se pun parolele: [docs/prin-internet.md](../docs/prin-internet.md).

APK-ul cu broker se face cu parola din `secrete.json` (ignorat de git, după `secrete.exemplu.json`):

```
flutter build apk --release --dart-define-from-file=secrete.json
```

Graficul de la Energie are și „3 zile”: punctele se țin pe telefon (SQLite), ~50 KB.

## Alarma: PIN, Face ID, Touch ID sau amprentă

PIN-ul îl verifică Arduino, nu aplicația. Prima dată dezarmăm cu PIN-ul; dacă Arduino l-a acceptat și
dezarmarea fără PIN e pornită, aplicația îl păstrează criptat pe telefon (Keychain pe iPhone, Keystore
pe Android). Data viitoare telefonul ne recunoaște și aplicația trimite ea PIN-ul.

Aplicația vede singură ce are telefonul și așa scrie peste tot:

- iPhone cu Face ID (X și mai noi) — „Dezarmare cu Face ID”;
- iPhone cu buton (SE, 8) — „Dezarmare cu Touch ID”;
- Android — „Dezarmare cu amprenta”.

Dacă nu merge (fața nu e recunoscută, telefonul nu are nimic setat), cerem PIN-ul.

## Pe iPhone

Aplicația pentru iPhone se poate face doar pe un Mac, așa că o face GitHub pe un Mac al lui: pe
GitHub, la **Actions → Aplicatia → Run workflow** (cu „iPhone” bifat). După vreo 10 minute, jos la
**Artifacts** apare `NEPHA-iphone`, cu `NEPHA.ipa` înăuntru.

Fișierul nu e semnat. Ca să îl punem pe telefon, de pe Windows:

1. instalăm [Sideloadly](https://sideloadly.io) și iTunes (de pe site-ul Apple, nu din Microsoft Store);
2. legăm iPhone-ul cu cablu, îl tragem pe `NEPHA.ipa` în Sideloadly și ne logăm cu Apple ID-ul nostru;
3. pe iPhone: **Setări → General → VPN și gestionare dispozitive** → avem încredere în Apple ID-ul nostru;
   pe iOS 16 și mai nou mai trebuie pornit și **Setări → Confidențialitate → Mod dezvoltator**.

Cu un Apple ID obișnuit (gratuit), aplicația merge 7 zile, apoi o punem din nou (datele rămân). Cu
contul de dezvoltator Apple (99 $ pe an) merge un an și se poate da și altora, prin TestFlight.

## Cum e făcută

- Vorbește cu panoul exact ca pagina casei: `GET /api/stare` la 2 secunde, `POST /api/comanda`
  (vezi [docs/protocol.md](../docs/protocol.md) și [panou/web.ino](../panou/web.ino)).
- Dacă panoul nu răspunde în 3 secunde, renunțăm și scriem „Panoul nu a putut fi contactat”.
- Comutatoarele se mută pe loc și așteaptă confirmarea; dacă nu vine, revin.
- Panoul vorbește HTTP simplu (fără HTTPS): pe Android e permis în `AndroidManifest.xml`
  (`usesCleartextTraffic`), pe iPhone în `Info.plist` (doar rețeaua locală).
- La prima deschidere, iPhone-ul întreabă dacă aplicația poate căuta în rețeaua locală (panoul) și,
  la prima dezarmare, dacă poate folosi Face ID: la amândouă răspundem „Permite”.
- Tema de noapte: culorile sunt în `lib/theme/nepha_theme.dart`, câte un set pentru zi și unul pentru
  noapte. Când se schimbă, `main.dart` reface tot ecranul ca să le citească din nou.
- Fontul Nunito e în aplicație (`assets/google_fonts`), merge și fără internet.

| Fișier | Ce face |
|---|---|
| `lib/models/api_models.dart` | ce trimite panoul (aceleași câmpuri ca în `panou/web.ino`) |
| `lib/services/api_service.dart` | cererile către panou pe Wi-Fi (dio, 3 s) și seara simulată pentru modul demo |
| `lib/services/broker.dart` | aceleași cereri prin brokerul MQTT, PIN-ul criptat |
| `lib/services/istoric_local.dart` | graficul pe 3 zile, pe telefon |
| `lib/providers/app_state.dart` | starea aplicației: Wi-Fi sau internet, întrebările periodice, comenzile, setările, Face ID / amprenta |
| `lib/theme/nepha_theme.dart` | culorile și fontul (aceleași ca pe pagină și pe panou) |
| `lib/widgets/` | cardurile, comutatoarele, graficul, harta rutinei, tastatura PIN |
| `lib/screens/` | cele patru ecrane |

Înainte de a urca modificări: `flutter analyze` și `flutter test`.
