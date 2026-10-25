// demo - o seara simulata, ca aplicatia sa se poata arata si fara panou
// (aceleasi valori ca modul demo al paginii casei). Comenzile chiar schimba starea.
import type { Decizie, Istoric, Rutina, Stare } from './tipuri';

const t0 = Date.now();
let mod: Stare['mod'] = 'smart';
let alarma = 0;
let armatLa = 0;
let pompaPanaLa = 0;
const lumini = [80, 0, 45];
const usi = [false, false, false, false];

const minutAcum = () => (18 * 60 + 40 + Math.floor((Date.now() - t0) / 1000)) % 1440; // un minut pe secunda
const ora = (m: number) =>
  String(Math.floor(m / 60) % 24).padStart(2, '0') + ':' + String(m % 60).padStart(2, '0');

const NUME_ALARMA = ['Dezarmată', 'Se armează...', 'Armată (noapte)', 'Armată (plecat)', 'Introduceți PIN-ul!', 'ALARMĂ!'];
const RUTINA = [
  [10, 11, 11, 11, 11, 11, 11, 88, 66, 11, 11, 11, 11, 11, 11, 11, 11, 77, 88, 88, 88, 88, 88, 11],
  [90, 88, 88, 88, 88, 88, 88, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 77, 88],
  [10, 11, 11, 11, 11, 11, 88, 88, 22, 11, 11, 11, 11, 11, 11, 11, 11, 11, 77, 88, 11, 11, 11, 11],
];

export const demo = {
  comanda(c: string): void {
    if (c === 'MOD SMART') mod = 'smart';
    if (c === 'MOD CLASIC') mod = 'clasic';
    if (c === 'ALARMA PLECAT') {
      alarma = 1; // ca pe casa adevarata: 30 de secunde ca sa iesim
      armatLa = Date.now() + 5000;
    }
    if (c === 'ALARMA ACASA') alarma = 2;
    if (c.startsWith('ALARMA OPRIT')) alarma = c.endsWith(' 1234') ? 0 : alarma;
    if (c === 'UDA') pompaPanaLa = Date.now() + 8000;
    const l = c.match(/^LUMINA (\d) (\d+)/);
    if (l) lumini[+l[1]] = +l[2];
    const u = c.match(/^USA (\d) (\d)/);
    if (u) usi[+u[1]] = u[2] === '1';
  },

  stare(): Stare {
    const m = minutAcum();
    const u = (Date.now() - t0) / 1000;
    if (alarma === 1 && Date.now() > armatLa) alarma = 3;
    return {
      primit: true,
      vechime: 0,
      ora: ora(m),
      minut: m,
      mod,
      test: 0,
      camere: [
        { nume: 'Sufragerie', temp: 21.4 + Math.sin(u / 20) * 0.2, lumina: lumini[0], prezenta: true, fereastra: false },
        { nume: 'Dormitor', temp: 19.6, lumina: lumini[1], prezenta: false, fereastra: usi[2] },
        { nume: 'Bucătărie', temp: 21.0, lumina: lumini[2], prezenta: true, fereastra: false },
      ],
      afara: 7.5,
      luminaNaturala: 8,
      incalzire: {
        pornita: Math.sin(u / 15) > 0,
        tinta: 21.5,
        motiv: 'e cineva acasă',
        motivCod: 1,
        rata: 3.4,
      },
      energie: {
        baterie: 78,
        solar: 0.08,
        vant: 0.31 + Math.sin(u / 7) * 0.05,
        consum: 1.2,
        mod: 'Normal',
        modCod: 0,
        whAzi: 5230 + u * 2,
        whClasic: 7480 + u * 3,
        mwh: 0,
        estimatSolar6h: 0.4,
        factorPanou: 0.82,
      },
      alarma: {
        stare: NUME_ALARMA[alarma],
        cod: alarma,
        senzori: [
          { nume: 'Ușa de la intrare', deschis: usi[0] },
          { nume: 'Ușa spre curte', deschis: usi[1] },
          { nume: 'Geam dormitor', deschis: usi[2] },
          { nume: 'Geam sufragerie', deschis: usi[3] },
        ],
      },
      gradina: { umiditate: 31, pompa: Date.now() < pompaPanaLa },
      priza: true,
      senzori: 159,
      vreme: { valida: true, temp: 7.8, ploaie: 80, oraPloaie: 21, soare: 0, nori: 90, vant: 14 },
      rezultate: [
        { valid: true, lumini: 177, incalzire: 16200, pompa: 60, priza: 397, total: 16834, mwh: 0, litri: 80 },
        { valid: true, lumini: 91, incalzire: 13333, pompa: 0, priza: 257, total: 13681, mwh: 0, litri: 0 },
      ],
      retea: { ip: '', nume: '', ap: false },
      pretKwh: 1.4,
    };
  },

  jurnal(): Decizie[] {
    return [
      { ora: '18:32', text: 'Am stins lumina în Bucătărie: nu mai era nimeni.' },
      { ora: '17:32', text: 'Alarma a fost dezarmată. Bine ați venit!' },
      { ora: '15:08', text: 'Pornesc încălzirea mai devreme: de obicei e cineva acasă pe la 17:00 (peste ~112 min).' },
      { ora: '08:12', text: 'Am oprit priza televizorului: stătea în standby de 16 min.' },
      { ora: '08:00', text: 'Alarma e armată. Casa știe că nu e nimeni și economisește.' },
      { ora: '06:48', text: 'În Bucătărie temperatura scade brusc, pare o fereastră deschisă. Opresc încălzirea 15 minute.' },
      { ora: '04:52', text: 'Pornesc încălzirea mai devreme: de obicei e cineva acasă pe la 6:00 (peste ~68 min).' },
      { ora: '00:04', text: 'Nu ud grădina: vine ploaia (80% pe la ora 16).' },
    ];
  },

  istoric(): Istoric {
    const minut: number[] = [], solar: number[] = [], vant: number[] = [], consum: number[] = [];
    for (let m = 0; m < 1130; m += 5) {
      const zi = Math.max(0, Math.sin(((m / 60 - 7) / 11) * Math.PI));
      minut.push(m);
      solar.push(+(1.9 * zi * (m > 960 && m < 1080 ? 0.35 : 1)).toFixed(2));
      vant.push(+(0.12 + 0.3 * Math.max(0, Math.sin(m / 190))).toFixed(2));
      const acasa = m < 480 || m > 1050;
      const calorifer = (acasa || m > 900) && Math.sin(m / 23) > 0.1 ? 1 : 0; // porneste si se opreste
      consum.push(+((acasa ? 0.45 : 0.15) + calorifer).toFixed(2));
    }
    return { minut, solar, vant, consum, tCasa: [], tAfara: [], baterie: [] };
  },

  rutina(): Rutina {
    return { valida: true, camere: ['Sufragerie', 'Dormitor', 'Bucătărie'], p: RUTINA, zile: Array(24).fill(7) };
  },
};
