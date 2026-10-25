// iconite - aceleasi desene ca pe pagina casei (linii simple, 24x24)
import type { ReactNode } from 'react';
import Svg, { Circle, Path, Rect } from 'react-native-svg';

import { culori } from '@/tema';

export type NumeIconita =
  | 'casa' | 'soare' | 'vant' | 'fulger' | 'frunza' | 'picatura'
  | 'scut' | 'termometru' | 'bec' | 'grila' | 'lista' | 'balanta';

const DESENE: Record<NumeIconita, ReactNode> = {
  casa: (
    <>
      <Path d="M3 11l9-7 9 7" />
      <Path d="M5 10v10h14V10" />
      <Path d="M10 20v-6h4v6" />
    </>
  ),
  soare: (
    <>
      <Circle cx={12} cy={12} r={4} />
      <Path d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4" />
    </>
  ),
  vant: (
    <>
      <Path d="M3 8h11a3 3 0 1 0-3-3" />
      <Path d="M3 12h16a3 3 0 1 1-3 3" />
      <Path d="M3 16h7" />
    </>
  ),
  fulger: <Path d="M13 2L4 14h7l-1 8 9-12h-7z" />,
  frunza: (
    <>
      <Path d="M5 19c0-9 6-14 15-14 0 9-5 15-14 15" />
      <Path d="M5 19l7-7" />
    </>
  ),
  picatura: <Path d="M12 3s6 6.5 6 11a6 6 0 0 1-12 0c0-4.5 6-11 6-11z" />,
  scut: (
    <>
      <Path d="M12 3l7 3v5c0 5-3 8-7 10-4-2-7-5-7-10V6z" />
      <Path d="M9 12l2 2 4-4" />
    </>
  ),
  termometru: (
    <>
      <Path d="M10 4a2 2 0 1 1 4 0v10a4 4 0 1 1-4 0z" />
      <Path d="M12 10v6" />
    </>
  ),
  bec: (
    <>
      <Path d="M9 18h6M10 21h4" />
      <Path d="M12 3a6 6 0 0 0-3.5 10.9c.6.5 1 1.2 1 2.1h5c0-.9.4-1.6 1-2.1A6 6 0 0 0 12 3z" />
    </>
  ),
  grila: (
    <>
      <Rect x={4} y={4} width={6} height={6} rx={1.5} />
      <Rect x={14} y={4} width={6} height={6} rx={1.5} />
      <Rect x={4} y={14} width={6} height={6} rx={1.5} />
      <Rect x={14} y={14} width={6} height={6} rx={1.5} />
    </>
  ),
  lista: (
    <>
      <Path d="M9 6h11M9 12h11M9 18h11" />
      <Circle cx={4.5} cy={6} r={1} />
      <Circle cx={4.5} cy={12} r={1} />
      <Circle cx={4.5} cy={18} r={1} />
    </>
  ),
  balanta: (
    <>
      <Path d="M12 4v16M5 20h14" />
      <Path d="M5 8h14" />
      <Path d="M5 8l-3 6a3 3 0 0 0 6 0zM19 8l-3 6a3 3 0 0 0 6 0z" />
    </>
  ),
};

export function Iconita({ nume, culoare = culori.text2, marime = 18 }: { nume: NumeIconita; culoare?: string; marime?: number }) {
  return (
    <Svg
      width={marime}
      height={marime}
      viewBox="0 0 24 24"
      fill="none"
      stroke={culoare}
      strokeWidth={1.9}
      strokeLinecap="round"
      strokeLinejoin="round">
      {DESENE[nume]}
    </Svg>
  );
}
