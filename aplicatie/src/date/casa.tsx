// casa - tot ce stie aplicatia despre casa, intr-un singur loc
//
// - intreaba panoul o data la 2 secunde (ca pagina casei); jurnalul, graficul si rutina mai rar
// - tine minte adresa panoului, modul demo si temperatura de confort
// - comenzile se vad pe loc ("optimist"): comutatorul se muta imediat, iar daca panoul
//   nu raspunde in 3 secunde revine si apare un mesaj
import AsyncStorage from '@react-native-async-storage/async-storage';
import { createContext, useCallback, useContext, useEffect, useMemo, useRef, useState, type ReactNode } from 'react';

import { panou } from './api';
import { demo } from './demo';
import type { Decizie, Istoric, Rutina, Stare } from './tipuri';

export type Conexiune = 'demo' | 'fara-adresa' | 'se-conecteaza' | 'conectat' | 'arduino-tace' | 'deconectat';

export interface Setari {
  adresa: string; // IP-ul panoului sau nepha.local
  demo: boolean;
  confort: number; // grade, trimise cu TEMP CONFORT
}

interface Mesaj {
  text: string;
  ton: 'info' | 'eroare';
  id: number;
  vizibil: boolean; // dupa cateva secunde se ascunde (dar textul ramane, ca sa se stinga frumos)
}

interface Optimist {
  valoare: unknown;
  pana: number; // dupa acest moment ne intoarcem la ce spune panoul
}

interface CasaContext {
  stare: Stare | null;
  jurnal: Decizie[];
  istoric: Istoric | null;
  rutina: Rutina | null;
  conexiune: Conexiune;
  setari: Setari;
  setariGata: boolean;
  schimbaSetari: (s: Partial<Setari>) => void;
  // trimite o comanda; cu `cheie`, interfata arata `valoare` pana confirma panoul
  comanda: (c: string, optimist?: { cheie: string; valoare: unknown }) => Promise<boolean>;
  arata: <T>(cheie: string, real: T) => T;
  asteapta: (cheie: string) => boolean;
  mesaj: Mesaj | null;
  anunta: (text: string, ton?: Mesaj['ton']) => void;
  improspateaza: () => void;
}

const Context = createContext<CasaContext | null>(null);
const CHEIE_SETARI = 'nepha.setari';
const SETARI_IMPLICITE: Setari = { adresa: '', demo: true, confort: 21.5 };
const PAS_MS = 2000;
const OPTIMIST_MS = 4500; // cat aratam valoarea noua inainte s-o vedem si de la panou

export function CasaProvider({ children }: { children: ReactNode }) {
  const [setari, setSetari] = useState<Setari>(SETARI_IMPLICITE);
  const [setariGata, setSetariGata] = useState(false);
  const [stare, setStare] = useState<Stare | null>(null);
  const [jurnal, setJurnal] = useState<Decizie[]>([]);
  const [istoric, setIstoric] = useState<Istoric | null>(null);
  const [rutina, setRutina] = useState<Rutina | null>(null);
  const [conexiune, setConexiune] = useState<Conexiune>('se-conecteaza');
  const [optimist, setOptimist] = useState<Record<string, Optimist>>({});
  const [asteptari, setAsteptari] = useState<Record<string, true>>({});
  const [mesaj, setMesaj] = useState<Mesaj | null>(null);
  const [, setCeas] = useState(0);
  const pas = useRef(0);
  const reia = useRef<() => void>(() => {});

  // setarile salvate
  useEffect(() => {
    AsyncStorage.getItem(CHEIE_SETARI)
      .then((t) => {
        if (t) setSetari({ ...SETARI_IMPLICITE, ...JSON.parse(t) });
      })
      .catch(() => {})
      .finally(() => setSetariGata(true));
  }, []);

  const schimbaSetari = useCallback((s: Partial<Setari>) => {
    setSetari((vechi) => {
      const nou = { ...vechi, ...s };
      AsyncStorage.setItem(CHEIE_SETARI, JSON.stringify(nou)).catch(() => {});
      return nou;
    });
    // alt panou (sau demo): ce stiam nu mai e valabil
    if (s.adresa !== undefined || s.demo !== undefined) {
      setStare(null);
      setConexiune('se-conecteaza');
    }
  }, []);

  const anunta = useCallback((text: string, ton: Mesaj['ton'] = 'info') => {
    setMesaj({ text, ton, id: Date.now(), vizibil: true });
  }, []);

  useEffect(() => {
    if (!mesaj?.vizibil) return;
    const t = setTimeout(() => setMesaj((m) => m && { ...m, vizibil: false }), 2800);
    return () => clearTimeout(t);
  }, [mesaj]);

  // intrebam panoul pe rand: starea la fiecare pas, restul mai rar
  useEffect(() => {
    if (!setariGata) return;
    let oprit = false;
    let ceas: ReturnType<typeof setTimeout> | undefined;
    pas.current = 0;

    const runda = async () => {
      const n = pas.current++;
      try {
        if (setari.demo) {
          setStare(demo.stare());
          if (n % 3 === 0) setJurnal(demo.jurnal());
          if (n % 15 === 0) setIstoric(demo.istoric());
          if (n % 30 === 0) setRutina(demo.rutina());
          setConexiune('demo');
        } else if (setari.adresa) {
          const s = await panou.stare(setari.adresa);
          if (oprit) return;
          setStare(s);
          const arduinoVorbeste = s.primit && s.vechime >= 0 && s.vechime < 10;
          setConexiune(arduinoVorbeste ? 'conectat' : 'arduino-tace');
          if (n % 3 === 0) panou.jurnal(setari.adresa).then((j) => !oprit && setJurnal(j)).catch(() => {});
          if (n % 15 === 0) panou.istoric(setari.adresa).then((i) => !oprit && setIstoric(i)).catch(() => {});
          if (n % 30 === 0) panou.rutina(setari.adresa).then((r) => !oprit && setRutina(r)).catch(() => {});
        }
      } catch {
        if (!oprit) {
          setConexiune('deconectat');
          pas.current = 0; // cand revine, luam din nou tot
        }
      }
      if (!oprit) ceas = setTimeout(runda, PAS_MS);
    };
    reia.current = () => {
      if (ceas) clearTimeout(ceas);
      runda();
    };
    runda();
    return () => {
      oprit = true;
      if (ceas) clearTimeout(ceas);
    };
  }, [setari.demo, setari.adresa, setariGata]);

  const improspateaza = useCallback(() => reia.current(), []);

  const comanda = useCallback(
    async (c: string, opt?: { cheie: string; valoare: unknown }) => {
      if (opt) {
        setOptimist((o) => ({ ...o, [opt.cheie]: { valoare: opt.valoare, pana: Date.now() + 60000 } }));
        setAsteptari((a) => ({ ...a, [opt.cheie]: true }));
      }
      let ok = true;
      try {
        if (setari.demo) {
          await new Promise((r) => setTimeout(r, 250)); // ca pe fir, nu chiar instant
          demo.comanda(c);
        } else {
          if (!setari.adresa) throw new Error('fara adresa');
          await panou.comanda(setari.adresa, c);
        }
      } catch {
        ok = false;
        anunta('Panoul nu a putut fi contactat', 'eroare');
      }
      if (opt) {
        setAsteptari(({ [opt.cheie]: _, ...rest }) => rest);
        setOptimist((o) => {
          if (!ok) {
            const { [opt.cheie]: __, ...rest } = o;
            return rest;
          }
          return { ...o, [opt.cheie]: { valoare: opt.valoare, pana: Date.now() + OPTIMIST_MS } };
        });
        if (ok) setTimeout(() => setCeas((x) => x + 1), OPTIMIST_MS + 50);
      }
      if (ok) setTimeout(() => reia.current(), 400); // aducem repede starea noua
      return ok;
    },
    [setari.demo, setari.adresa, anunta],
  );

  const arata = useCallback(
    <T,>(cheie: string, real: T): T => {
      const o = optimist[cheie];
      return o && o.pana > Date.now() ? (o.valoare as T) : real;
    },
    [optimist],
  );

  const asteapta = useCallback((cheie: string) => !!asteptari[cheie], [asteptari]);

  // fara adresa (si fara demo) nu avem pe cine intreba
  const conexiuneAcum: Conexiune = setari.demo ? 'demo' : !setari.adresa ? 'fara-adresa' : conexiune;

  const valoare = useMemo<CasaContext>(
    () => ({
      stare,
      jurnal,
      istoric,
      rutina,
      conexiune: conexiuneAcum,
      setari,
      setariGata,
      schimbaSetari,
      comanda,
      arata,
      asteapta,
      mesaj,
      anunta,
      improspateaza,
    }),
    [stare, jurnal, istoric, rutina, conexiuneAcum, setari, setariGata, schimbaSetari, comanda, arata, asteapta, mesaj, anunta, improspateaza],
  );

  return <Context.Provider value={valoare}>{children}</Context.Provider>;
}

export function useCasa(): CasaContext {
  const c = useContext(Context);
  if (!c) throw new Error('useCasa trebuie folosit in CasaProvider');
  return c;
}
