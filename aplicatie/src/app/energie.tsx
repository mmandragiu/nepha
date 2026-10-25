// Energie - de unde vine curentul acum, graficul zilei si "ziua de test" (clasic vs smart)
import type { ReactNode } from 'react';
import { StyleSheet, View } from 'react-native';

import { GraficEnergie } from '@/componente/grafice';
import { Bara, Buton, Card, Ecran, Eticheta, IndicatorConexiune, Pastila, T } from '@/componente/ui';
import { useCasa } from '@/date/casa';
import { EXPLICATIE_ENERGIE, nr } from '@/date/texte';
import type { RezultatTest } from '@/date/tipuri';
import { culori } from '@/tema';

const TON_MOD = ['verde', 'soare', 'cald', 'rosu'] as const;

export default function Energie() {
  const { stare, istoric, comanda } = useCasa();
  if (!stare) {
    return (
      <Ecran titlu="Energie" dreapta={<IndicatorConexiune />}>
        <T fel="mic">Se așteaptă datele de la casă...</T>
      </Ecran>
    );
  }
  const en = stare.energie;
  const culoareBaterie = en.modCod === 3 ? culori.rosu : en.modCod === 2 ? culori.soare : culori.verde;
  const [c, s] = stare.rezultate;
  const avemTest = c?.valid && s?.valid;

  return (
    <Ecran titlu="Energie" subtitlu="la scara machetei, în wați" dreapta={<IndicatorConexiune />}>
      <Card>
        <Eticheta iconita="fulger" culoare={culori.soare} dreapta={<Pastila ton={TON_MOD[en.modCod] ?? 'gri'}>{`Mod ${en.mod.toLowerCase()}`}</Pastila>}>
          Acum
        </Eticheta>
        <View style={stil.grila}>
          <Bucata eticheta="Soare" valoare={`${nr(en.solar, 2)} W`} />
          <Bucata eticheta="Vânt" valoare={`${nr(en.vant, 2)} W`} />
          <Bucata eticheta="Baterie" valoare={`${en.baterie}%`}>
            <View style={{ marginTop: 8 }}>
              <Bara procent={en.baterie} culoare={culoareBaterie} />
            </View>
          </Bucata>
          <Bucata eticheta="Consum" valoare={`${nr(en.consum, 2)} W`} />
        </View>
        <T fel="mic" style={{ marginTop: 12 }}>
          {EXPLICATIE_ENERGIE[en.modCod] ?? ''}
        </T>
      </Card>

      <Card>
        <Eticheta iconita="soare" culoare={culori.soare}>
          Azi
        </Eticheta>
        <GraficEnergie istoric={istoric} />
        {en.estimatSolar6h != null ? (
          <T fel="mic" style={{ marginTop: 8 }}>
            În următoarele 6 ore casa estimează ~{nr(en.estimatSolar6h, 1)} Wh din soare.
          </T>
        ) : null}
      </Card>

      <Card>
        <Eticheta iconita="balanta" culoare={culori.verde}>
          Ziua de test
        </Eticheta>
        <T fel="mic" style={{ marginBottom: 12 }}>
          Aceeași zi, trăită o dată ca o casă clasică și o dată ca o casă NEPHA. Aceeași familie, aceeași vreme.
        </T>
        {avemTest ? <RezultateTest c={c} s={s} /> : <T fel="mic">Încă n-am rulat ziua de test. Apăsați „Calculează”.</T>}
        <View style={stil.butoane}>
          <Buton principal onPress={() => comanda('TEST RAPID')} style={{ flex: 1 }}>
            Calculează
          </Buton>
          <Buton onPress={() => comanda(stare.test === 1 ? 'TEST STOP' : 'TEST SMART')} style={{ flex: 1 }}>
            {stare.test === 1 ? 'Oprește testul' : 'Pe machetă'}
          </Buton>
        </View>
      </Card>
    </Ecran>
  );
}

function Bucata({ eticheta, valoare, children }: { eticheta: string; valoare: string; children?: ReactNode }) {
  return (
    <View style={stil.bucata}>
      <T fel="mic">{eticheta}</T>
      <T style={stil.valoare}>{valoare}</T>
      {children}
    </View>
  );
}

function RezultateTest({ c, s }: { c: RezultatTest; s: RezultatTest }) {
  const pct = (a: number, b: number) => (a > 0 ? Math.round(((a - b) / a) * 100) : 0);
  const randuri: [string, number, number][] = [
    ['Încălzire', c.incalzire, s.incalzire],
    ['Lumini', c.lumini, s.lumini],
    ['Priza TV (standby)', c.priza, s.priza],
    ['Udare', c.pompa, s.pompa],
  ];
  return (
    <View style={{ gap: 8 }}>
      <View style={stil.randMare}>
        <T fel="mare" style={{ color: culori.verde }}>
          −{pct(c.total, s.total)}%
        </T>
        <T fel="mic" style={{ flex: 1 }}>
          {nr(c.total / 1000, 1)} → {nr(s.total / 1000, 1)} kWh pe zi · {nr(c.litri, 0)} → {nr(s.litri, 0)} litri de apă
        </T>
      </View>
      {randuri.map(([n, a, b]) => (
        <View key={n} style={stil.randTest}>
          <T fel="mic" style={{ flex: 1 }}>
            {n}
          </T>
          <T fel="mic">
            <T fel="mic" style={{ color: culori.text, fontFamily: 'Nunito_700Bold' }}>
              −{pct(a, b)}%
            </T>{' '}
            · {nr(a, 0)} → {nr(b, 0)} Wh
          </T>
        </View>
      ))}
      {c.mwh > 0 ? (
        <T fel="mic">
          Măsurat pe machetă (INA219): {nr(c.mwh, 0)} → {nr(s.mwh, 0)} mWh
        </T>
      ) : null}
    </View>
  );
}

const stil = StyleSheet.create({
  grila: { flexDirection: 'row', flexWrap: 'wrap', gap: 10 },
  bucata: { width: '48%', flexGrow: 1, backgroundColor: culori.moale, borderRadius: 12, padding: 12 },
  valoare: { fontFamily: 'Nunito_700Bold', fontSize: 22, marginTop: 2 },
  randMare: { flexDirection: 'row', alignItems: 'center', gap: 12 },
  randTest: { flexDirection: 'row', justifyContent: 'space-between', gap: 10 },
  butoane: { flexDirection: 'row', gap: 8, marginTop: 14 },
});
