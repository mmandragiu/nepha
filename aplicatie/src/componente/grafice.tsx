// grafice - graficul de energie, inelul gradinii si harta rutinei (fara gradienti)
import { useState } from 'react';
import { StyleSheet, View } from 'react-native';
import Svg, { Circle, Path } from 'react-native-svg';

import type { Istoric, Rutina } from '@/date/tipuri';
import { culori } from '@/tema';
import { T } from './ui';

// ---------------------------------------------------------------- energie pe ore

// media pe cateva puncte vecine: caloriferul porneste si se opreste des, iar linia zimtata nu se citeste
function netezeste(v: number[], k = 3): number[] {
  return v.map((_, i) => {
    const b = v.slice(Math.max(0, i - k), i + k + 1);
    return b.reduce((a, x) => a + x, 0) / b.length;
  });
}

export function GraficEnergie({ istoric }: { istoric: Istoric | null }) {
  const [latime, setLatime] = useState(0);
  const H = 140;
  const n = istoric?.minut.length ?? 0;
  if (!istoric || n < 2) {
    return (
      <View style={[stil.gol, { height: H }]}>
        <T fel="mic">graficul apare după câteva minute</T>
      </View>
    );
  }
  const prod = istoric.solar.map((s, i) => s + (istoric.vant[i] ?? 0));
  const consum = netezeste(istoric.consum);
  const max = Math.max(0.5, ...prod, ...consum) * 1.15;
  const x = (i: number) => (i / (n - 1)) * latime;
  const y = (v: number) => H - 4 - (v / max) * (H - 12);
  const linie = (v: number[]) => v.map((a, i) => `${i ? 'L' : 'M'}${x(i).toFixed(1)} ${y(a).toFixed(1)}`).join(' ');

  // orele de sub grafic, din 3 in 3
  const ore: { p: number; text: string }[] = [];
  for (let i = 1; i < n; i++) {
    const m = istoric.minut[i];
    if (m % 180 < 5 && (istoric.minut[i - 1] % 180 >= 5 || istoric.minut[i - 1] > m)) {
      const p = i / (n - 1);
      if (p > 0.04 && p < 0.96) ore.push({ p, text: `${Math.floor(m / 60)}:00` });
    }
  }

  return (
    <View onLayout={(e) => setLatime(e.nativeEvent.layout.width)}>
      {latime > 0 ? (
        <Svg width={latime} height={H}>
          <Path d={`${linie(prod)} L${latime} ${H} L0 ${H} Z`} fill={culori.soare} fillOpacity={0.12} />
          <Path d={linie(prod)} stroke={culori.soare} strokeWidth={2} fill="none" />
          <Path d={linie(consum)} stroke={culori.text} strokeWidth={1.5} strokeOpacity={0.8} fill="none" />
        </Svg>
      ) : (
        <View style={{ height: H }} />
      )}
      <View style={{ height: 16 }}>
        {ore.map((o) => (
          <T key={o.text + o.p} fel="mic" style={[stil.ora, { left: o.p * latime - 16 }]}>
            {o.text}
          </T>
        ))}
      </View>
      <View style={stil.legenda}>
        <View style={stil.legendaRand}>
          <View style={[stil.patrat, { backgroundColor: culori.soare }]} />
          <T fel="mic">producție (soare + vânt)</T>
        </View>
        <View style={stil.legendaRand}>
          <View style={[stil.patrat, { backgroundColor: culori.text }]} />
          <T fel="mic">consum</T>
        </View>
      </View>
    </View>
  );
}

// ---------------------------------------------------------------- inelul gradinii

export function Inel({ procent, marime = 120, culoare = culori.apa }: { procent: number; marime?: number; culoare?: string }) {
  const g = 12;
  const r = (marime - g) / 2;
  const L = 2 * Math.PI * r;
  const p = Math.max(0, Math.min(100, procent));
  return (
    <View style={{ width: marime, height: marime, alignItems: 'center', justifyContent: 'center' }}>
      <Svg width={marime} height={marime} style={StyleSheet.absoluteFill}>
        <Circle cx={marime / 2} cy={marime / 2} r={r} stroke={culori.moale} strokeWidth={g} fill="none" />
        <Circle
          cx={marime / 2}
          cy={marime / 2}
          r={r}
          stroke={culoare}
          strokeWidth={g}
          fill="none"
          strokeDasharray={`${(L * p) / 100} ${L}`}
          transform={`rotate(-90 ${marime / 2} ${marime / 2})`}
        />
      </Svg>
      <T fel="mediu">{Math.round(p)}%</T>
      <T fel="mic">umiditate</T>
    </View>
  );
}

// ---------------------------------------------------------------- ce a invatat casa

export function HartaRutina({ rutina, oraAcum }: { rutina: Rutina | null; oraAcum: number }) {
  if (!rutina) return <T fel="mic">Se încarcă...</T>;
  return (
    <View style={{ gap: 4 }}>
      {rutina.camere.map((nume, i) => (
        <View key={nume} style={stil.randHarta}>
          <T fel="mic" style={stil.numeHarta} numberOfLines={1}>
            {nume}
          </T>
          <View style={stil.celule}>
            {rutina.p[i].map((p, h) => (
              <View key={h} style={[stil.locCelula, h === oraAcum && stil.celulaAcum]}>
                <View style={[stil.celula, { opacity: 0.08 + (Math.max(0, p - 10) / 90) * 0.92 }]} />
              </View>
            ))}
          </View>
        </View>
      ))}
      <View style={stil.randHarta}>
        <View style={stil.numeHarta} />
        <View style={stil.celule}>
          {Array.from({ length: 24 }, (_, h) => (
            <T key={h} style={stil.oraHarta}>
              {h % 6 === 0 ? h : ''}
            </T>
          ))}
        </View>
      </View>
    </View>
  );
}

const stil = StyleSheet.create({
  gol: { alignItems: 'center', justifyContent: 'center' },
  ora: { position: 'absolute', width: 32, textAlign: 'center', fontSize: 11 },
  legenda: { flexDirection: 'row', gap: 14, marginTop: 4, flexWrap: 'wrap' },
  legendaRand: { flexDirection: 'row', alignItems: 'center', gap: 6 },
  patrat: { width: 10, height: 10, borderRadius: 3 },
  randHarta: { flexDirection: 'row', alignItems: 'center', gap: 6 },
  numeHarta: { width: 74 },
  celule: { flex: 1, flexDirection: 'row', gap: 2 },
  locCelula: { flex: 1, height: 22, borderRadius: 5, borderWidth: 1.5, borderColor: 'transparent' },
  celula: { flex: 1, borderRadius: 3, backgroundColor: culori.verde },
  celulaAcum: { borderColor: culori.text },
  oraHarta: { flex: 1, fontSize: 10, color: culori.text2, textAlign: 'left' },
});
