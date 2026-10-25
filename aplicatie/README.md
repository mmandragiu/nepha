# NEPHA — aplicația de telefon

Aceeași casă ca pe panou și pe pagina casei, dar ca aplicație pentru Android și iPhone. Aplicația nu
ia nicio decizie: doar arată ce face casa și trimite comenzile noastre, prin panoul ESP32. Creierul
rămâne Arduino.

Ecrane: **Acasă** (economia de azi, în casă / afară, baterie, alarmă, grădină, jurnalul deciziilor),
**Camere** (luminile și ce a învățat casa), **Energie** (graficul zilei și ziua de test), **Alarmă**
(armare, PIN, uși și geamuri) și **Setări** (adresa panoului, modul demo, Smart / Clasic).

## Cum o pornim

Avem nevoie de [Node.js](https://nodejs.org) pe calculator și de aplicația **Expo Go** pe telefon
(Google Play / App Store).

```
cd aplicatie
npm install
npx expo start
```

Apare un cod QR: îl scanăm cu Expo Go (Android) sau cu camera (iPhone). Telefonul și calculatorul
trebuie să fie în aceeași rețea Wi-Fi.

## Cum o legăm de casă

1. Telefonul în aceeași rețea Wi-Fi cu panoul (sau în rețeaua panoului, `NEPHA-casa`, parola `casa2026`).
2. Pe panou, la **Setări**, scrie adresa lui (de exemplu `192.168.1.42`; în rețeaua `NEPHA-casa` e
   `192.168.4.1`).
3. În aplicație, la **Setări**: scriem adresa, apăsăm **Conectează**.

Sus, în dreapta, se vede legătura: verde = conectat, galben = se conectează sau Arduino nu răspunde,
roșu = panoul nu răspunde.

Fără panou, **modul demo** arată o seară simulată (e pornit la prima deschidere). Comenzile merg și
acolo, ca să se poată arăta aplicația oriunde.

## Cum e făcută

- [Expo](https://expo.dev) (SDK 58) + Expo Router, TypeScript.
- Vorbește cu panoul exact ca pagina casei: `GET /api/stare` la 2 secunde, `POST /api/comanda`
  (vezi [docs/protocol.md](../docs/protocol.md) și [panou/web.ino](../panou/web.ino)).
- Dacă panoul nu răspunde în 3 secunde, renunțăm și scriem „Panoul nu a putut fi contactat”.
- Comutatoarele se mută pe loc și așteaptă confirmarea; dacă nu vine, revin.

| Fișier | Ce face |
|---|---|
| `src/date/tipuri.ts` | ce trimite panoul (aceleași câmpuri ca în `panou/web.ino`) |
| `src/date/api.ts` | cererile către panou, cu timp de așteptare |
| `src/date/casa.tsx` | starea aplicației: întrebările periodice, comenzile, setările |
| `src/date/demo.ts` | seara simulată (aceeași ca pe pagina casei) |
| `src/componente/` | cardurile, comutatoarele, graficul, tastatura PIN, filele de jos |
| `src/app/` | ecranele |
| `src/tema.ts` | culorile și fontul (aceleași ca pe pagină și pe panou) |

Înainte de a urca modificări: `npx tsc --noEmit`, `npx eslint src` și `npx expo-doctor`.
