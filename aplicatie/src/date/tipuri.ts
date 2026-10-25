// tipuri - exact ce trimite panoul (panou/web.ino). Unde Arduino nu stie o valoare
// (de ex. o camera fara termometru), JSON-ul are null.

export interface Camera {
  nume: string;
  temp: number | null;
  lumina: number; // 0-100
  prezenta: boolean;
  fereastra: boolean;
}

export interface SenzorUsa {
  nume: string;
  deschis: boolean;
}

export interface RezultatTest {
  valid: boolean;
  lumini: number; // Wh
  incalzire: number;
  pompa: number;
  priza: number;
  total: number;
  mwh: number; // masurat pe macheta (INA219)
  litri: number;
}

// cod alarma: 0 dezarmata, 1 se armeaza, 2 armata noapte, 3 armata plecat, 4 cere PIN, 5 suna
export interface Stare {
  primit: boolean; // panoul primeste date de la Arduino?
  vechime: number; // de cate secunde n-a mai venit nimic de la Arduino (-1 = niciodata)
  ora: string;
  minut: number;
  mod: 'smart' | 'clasic';
  test: number; // 0 nu, 1 ziua de test pe macheta, 2 test rapid
  camere: Camera[];
  afara: number | null;
  luminaNaturala: number;
  incalzire: {
    pornita: boolean;
    tinta: number;
    motiv: string;
    motivCod: number;
    rata: number; // grade pe ora, invatat
  };
  energie: {
    baterie: number; // %
    solar: number; // W (la scara machetei)
    vant: number;
    consum: number;
    mod: string; // Normal, Surplus, Economie, Critic
    modCod: number; // 0..3 in aceeasi ordine
    whAzi: number;
    whClasic: number; // cat ar fi consumat o casa clasica azi
    mwh: number;
    estimatSolar6h: number | null;
    factorPanou: number;
  };
  alarma: { stare: string; cod: number; senzori: SenzorUsa[] };
  gradina: { umiditate: number; pompa: boolean };
  priza: boolean;
  senzori: number;
  vreme: {
    valida: boolean;
    temp?: number;
    ploaie?: number;
    oraPloaie?: number;
    soare?: number;
    nori?: number;
    vant?: number;
  };
  rezultate: RezultatTest[]; // [0] clasic, [1] smart
  retea: { ip: string; nume: string; ap: boolean };
  pretKwh: number;
}

export interface Decizie {
  ora: string;
  cod?: number;
  text: string;
}

export interface Istoric {
  minut: number[];
  tCasa: (number | null)[];
  tAfara: (number | null)[];
  baterie: number[];
  solar: number[];
  vant: number[];
  consum: number[];
}

export interface Rutina {
  valida: boolean;
  camere: string[];
  p: number[][]; // [camera][ora] = % sanse sa fie cineva
  zile: number[];
}
