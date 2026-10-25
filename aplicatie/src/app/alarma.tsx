// Alarma - armare / dezarmare cu PIN si usile / geamurile.
// Telecomanda de pe o usa se mai opreste singura: atingand starea, o punem cum e de fapt.
import { useState } from 'react';
import { StyleSheet, View } from 'react-native';

import { Iconita } from '@/componente/iconite';
import { TastaturaPin } from '@/componente/tastatura-pin';
import { Buton, Card, Ecran, Eticheta, IndicatorConexiune, Pastila, T } from '@/componente/ui';
import { useCasa } from '@/date/casa';
import { EXPLICATIE_ALARMA } from '@/date/texte';
import { culori } from '@/tema';

export default function Alarma() {
  const { stare, comanda, arata, asteapta } = useCasa();
  const [pin, setPin] = useState(false);

  if (!stare) {
    return (
      <Ecran titlu="Alarmă" dreapta={<IndicatorConexiune />}>
        <T fel="mic">Se așteaptă datele de la casă...</T>
      </Ecran>
    );
  }
  const al = stare.alarma;
  const pericol = al.cod >= 4;
  const culoare = pericol ? culori.rosu : al.cod === 0 ? culori.text2 : culori.verde;

  return (
    <Ecran titlu="Alarmă" dreapta={<IndicatorConexiune />}>
      <Card style={pericol && stil.pericol}>
        <View style={stil.stare}>
          <Iconita nume="scut" culoare={culoare} marime={30} />
          <T fel="mediu" style={[{ flex: 1 }, pericol && { color: culori.rosu }]}>
            {al.stare}
          </T>
        </View>
        <T fel="mic" style={{ marginTop: 8 }}>
          {EXPLICATIE_ALARMA[al.cod] ?? ''}
        </T>
        <View style={stil.butoane}>
          {al.cod === 0 ? (
            <>
              <Buton principal onPress={() => comanda('ALARMA PLECAT')} style={{ flex: 1 }}>
                Plec de acasă
              </Buton>
              <Buton onPress={() => comanda('ALARMA ACASA')} style={{ flex: 1 }}>
                Noapte
              </Buton>
            </>
          ) : (
            <Buton principal onPress={() => setPin(true)} style={{ flex: 1 }}>
              Dezarmează
            </Buton>
          )}
        </View>
      </Card>

      <Card>
        <Eticheta iconita="casa">Uși și geamuri</Eticheta>
        {al.senzori.map((s, i) => {
          const cheie = `usa:${i}`;
          const deschis = arata(cheie, s.deschis);
          return (
            <View key={s.nume} style={[stil.senzor, i > 0 && stil.cuLinie]}>
              <T style={{ flex: 1 }}>{s.nume}</T>
              <Pastila
                ton={deschis ? 'cald' : 'gri'}
                asteapta={asteapta(cheie)}
                onPress={() => comanda(`USA ${i} ${deschis ? 0 : 1}`, { cheie, valoare: !deschis })}>
                {deschis ? 'deschis' : 'închis'}
              </Pastila>
            </View>
          );
        })}
        <T fel="mic" style={{ marginTop: 10 }}>
          Dacă o stare nu e cea reală (telecomanda de pe ușă s-a oprit singură), atingeți-o ca s-o schimbați. Rămâne așa
          până transmite senzorul din nou și nu pornește alarma.
        </T>
      </Card>

      <TastaturaPin
        vizibil={pin}
        onInchide={() => setPin(false)}
        onTrimite={(p) => {
          setPin(false);
          comanda('ALARMA OPRIT ' + p);
        }}
      />
    </Ecran>
  );
}

const stil = StyleSheet.create({
  pericol: { borderColor: culori.rosu, borderWidth: 1.5, backgroundColor: '#FFF8F8' },
  stare: { flexDirection: 'row', alignItems: 'center', gap: 12 },
  butoane: { flexDirection: 'row', gap: 8, marginTop: 16 },
  senzor: { flexDirection: 'row', alignItems: 'center', gap: 10, paddingVertical: 10 },
  cuLinie: { borderTopWidth: StyleSheet.hairlineWidth, borderTopColor: culori.linie },
});
