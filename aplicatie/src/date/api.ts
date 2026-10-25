// api - vorbim cu panoul pe HTTP, la fel ca pagina casei (vezi docs/protocol.md)
//   GET  /api/stare, /api/jurnal, /api/istoric, /api/rutina
//   POST /api/comanda   c=<comanda>   (ex. "LUMINA 0 100", "ALARMA PLECAT")
import type { Decizie, Istoric, Rutina, Stare } from './tipuri';

const ASTEPT_MS = 3000; // daca panoul nu raspunde in 3 s, renuntam

// "192.168.1.42" sau "nepha.local/" -> "http://192.168.1.42"
export function adresaCompleta(adresa: string): string {
  let a = adresa.trim().replace(/\/+$/, '');
  if (a && !/^https?:\/\//i.test(a)) a = 'http://' + a;
  return a;
}

async function cuTimp(url: string, optiuni: RequestInit = {}): Promise<Response> {
  const oprire = new AbortController();
  const ceas = setTimeout(() => oprire.abort(), ASTEPT_MS);
  try {
    const r = await fetch(url, { ...optiuni, signal: oprire.signal });
    if (!r.ok) throw new Error(`panoul a raspuns ${r.status}`);
    return r;
  } finally {
    clearTimeout(ceas);
  }
}

async function ia<T>(adresa: string, cale: string): Promise<T> {
  const r = await cuTimp(adresaCompleta(adresa) + cale, { headers: { 'Cache-Control': 'no-store' } });
  return (await r.json()) as T;
}

export const panou = {
  stare: (adresa: string) => ia<Stare>(adresa, '/api/stare'),
  jurnal: (adresa: string) => ia<Decizie[]>(adresa, '/api/jurnal'),
  istoric: (adresa: string) => ia<Istoric>(adresa, '/api/istoric'),
  rutina: (adresa: string) => ia<Rutina>(adresa, '/api/rutina'),
  async comanda(adresa: string, c: string): Promise<void> {
    await cuTimp(adresaCompleta(adresa) + '/api/comanda', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'c=' + encodeURIComponent(c),
    });
  },
};
