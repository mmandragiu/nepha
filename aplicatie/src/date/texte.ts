// texte - numere scrise ca la noi (21,4) si explicatiile de pe panou
import type { Stare } from './tipuri';

export function nr(x: number | null | undefined, zecimale = 1): string {
  if (x === null || x === undefined || Number.isNaN(x)) return '–';
  return x.toLocaleString('ro-RO', { minimumFractionDigits: zecimale, maximumFractionDigits: zecimale });
}

export function grade(x: number | null | undefined): string {
  return x === null || x === undefined || Number.isNaN(x) ? '–' : nr(x, 1) + '°';
}

export function salut(minut: number): string {
  const h = Math.floor(minut / 60);
  return h >= 5 && h < 11 ? 'Bună dimineața' : h >= 11 && h < 18 ? 'Bună ziua' : 'Bună seara';
}

export function tempCasa(s: Stare): number | null {
  const t = s.camere.map((c) => c.temp).filter((x): x is number => x !== null && x !== undefined);
  return t.length ? t.reduce((a, b) => a + b, 0) / t.length : null;
}

export function kwhEconomisiti(s: Stare): number {
  return Math.max(0, ((s.energie.whClasic || 0) - (s.energie.whAzi || 0)) / 1000);
}

export function textVreme(s: Stare): string {
  const v = s.vreme;
  if (!v.valida) return 'Fără prognoză (panoul nu are internet)';
  if (v.oraPloaie !== undefined) return `Ploaie ${v.ploaie}% pe la ${v.oraPloaie}:00`;
  return (v.soare ?? 0) > 40 ? 'Senin în următoarele ore' : 'Fără ploaie în 12 ore';
}

export const EXPLICATIE_ALARMA = [
  'Când plecați, armați-o: casa știe atunci sigur că e goală și economisește.',
  'Aveți 30 de secunde să ieșiți și să închideți ușa.',
  'Suntem acasă și dormim: păzim doar ușile și geamurile.',
  'Casa e păzită și economisește: lumini stinse, încălzire redusă, priza TV oprită.',
  'S-a deschis ușa. Introduceți PIN-ul!',
  'Alarma sună! Introduceți PIN-ul ca s-o opriți.',
];

export const EXPLICATIE_ENERGIE = [
  'Energie destulă. Casa merge normal.',
  'Bateria e plină și panoul încă produce: pornim acum ce poate aștepta.',
  'Bateria scade și nu se produce: lumini mai slabe, un grad mai puțin.',
  'Bateria e aproape goală: doar strictul necesar.',
];
