// secrete.exemplu.h - modelul pentru secrete.h, unde stau cheile si parolele panoului
//
// 1. Faceti o copie a fisierului asta, in acelasi folder (panou/), cu numele secrete.h
// 2. In secrete.h scrieti valorile adevarate.
// secrete.h e in .gitignore: nu ajunge pe GitHub. Fisierul asta (exemplul) se urca,
// deci aici NU scrieti nicio cheie sau parola adevarata.

// Wi-Fi-ul casei (optional: se poate alege si de pe telefon, din reteaua NEPHA-casa)
// #define WIFI_NUME   "numele retelei"
// #define WIFI_PAROLA "parola retelei"

// cheile de inrolare de la serverul de conturi al brokerului (nepha-auth.mandragiu.eu).
// Cu ele panoul isi face cont pe broker si le face cont telefoanelor legate de el.
// Fara ele, panoul merge doar pe Wi-Fi.
#define INROLARE_PANOURI   "cheia de inrolare a panourilor (X-Enroll-Key pentru /enroll/server)"
#define INROLARE_TELEFOANE "cheia de inrolare a telefoanelor (X-Enroll-Key pentru /enroll/client)"
#define INROLARE_RESET     "cheia de resetare (cand un id are deja cont si ne trebuie parola noua)"
