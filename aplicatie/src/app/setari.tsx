// Setari - unde e panoul, modul demo, cum se poarta casa
import { useState } from 'react';
import { Platform, Pressable, StyleSheet, TextInput, View } from 'react-native';

import { atingere, Buton, Card, Comutator, Ecran, Eticheta, IndicatorConexiune, T } from '@/componente/ui';
import { useCasa } from '@/date/casa';
import { nr } from '@/date/texte';
import { colt, culori, fonturi } from '@/tema';

export default function Setari() {
  const { stare, setari, schimbaSetari, comanda, arata, asteapta, anunta } = useCasa();
  const [adresa, setAdresa] = useState(setari.adresa);
  // fara date nu stim modul: niciun buton nu pare ales
  const smart = stare ? arata('mod', stare.mod === 'smart') : null;
  const grade = arata('confort', setari.confort);

  const conecteaza = () => {
    const a = adresa.trim();
    if (!a) {
      anunta('Scrieți adresa panoului (o vedeți pe ecranul lui, la Setări)', 'eroare');
      return;
    }
    schimbaSetari({ adresa: a, demo: false });
    anunta('Mă conectez la ' + a);
  };

  const confort = async (pas: number) => {
    const nou = Math.max(16, Math.min(26, Math.round((grade + pas) * 2) / 2));
    // o tinem minte doar daca a ajuns la casa
    if (await comanda(`TEMP CONFORT ${Math.round(nou * 10)}`, { cheie: 'confort', valoare: nou })) schimbaSetari({ confort: nou });
  };

  return (
    <Ecran titlu="Setări" dreapta={<IndicatorConexiune />}>
      <Card>
        <Eticheta iconita="casa">Panoul casei</Eticheta>
        <T fel="mic">Adresa e scrisă pe ecranul panoului, la Setări (de exemplu 192.168.1.42). Telefonul trebuie să fie în aceeași rețea Wi-Fi.</T>
        <TextInput
          value={adresa}
          onChangeText={setAdresa}
          placeholder="192.168.1.42"
          placeholderTextColor={culori.text2}
          autoCapitalize="none"
          autoCorrect={false}
          keyboardType={Platform.OS === 'ios' ? 'numbers-and-punctuation' : 'url'}
          onSubmitEditing={conecteaza}
          style={stil.camp}
        />
        <Buton principal onPress={conecteaza}>
          Conectează
        </Buton>
        <View style={[stil.rand, { marginTop: 14 }]}>
          <View style={{ flex: 1 }}>
            <T>Mod demo</T>
            <T fel="mic">o seară simulată, fără panou (pentru prezentări)</T>
          </View>
          <Comutator pornit={setari.demo} culoare={culori.verde} eticheta="Mod demo" onSchimba={(d) => schimbaSetari({ demo: d })} />
        </View>
      </Card>

      <Card>
        <Eticheta iconita="frunza" culoare={culori.verde}>
          Cum se poartă casa
        </Eticheta>
        <View style={stil.segment}>
          {(['smart', 'clasic'] as const).map((m) => {
            const activ = smart !== null && (m === 'smart') === smart;
            return (
              <Pressable
                key={m}
                onPress={() => {
                  atingere();
                  if (!activ) comanda(m === 'smart' ? 'MOD SMART' : 'MOD CLASIC', { cheie: 'mod', valoare: m === 'smart' });
                }}
                style={[stil.segmentButon, activ && stil.segmentActiv]}>
                <T style={[stil.segmentText, activ && { color: culori.text }]}>
                  {m === 'smart' ? 'Smart' : 'Clasic'}
                  {activ && asteapta('mod') ? '…' : ''}
                </T>
              </Pressable>
            );
          })}
        </View>
        <T fel="mic" style={{ marginTop: 8 }}>
          {smart === null
            ? 'Modul apare când aplicația vorbește cu casa.'
            : smart
              ? 'Casa ia singură deciziile care economisesc energie.'
              : 'Casa merge ca una obișnuită (pentru comparație).'}
        </T>

        <View style={[stil.rand, { marginTop: 16 }]}>
          <View style={{ flex: 1 }}>
            <T>Temperatura de confort</T>
            <T fel="mic">cât de cald să fie când e cineva acasă</T>
          </View>
          <Buton onPress={() => confort(-0.5)} style={stil.pas}>
            −
          </Buton>
          <T style={stil.grade}>{nr(grade, 1)}°</T>
          <Buton onPress={() => confort(0.5)} style={stil.pas}>
            +
          </Buton>
        </View>
      </Card>

      {stare && stare.retea.ip ? (
        <Card>
          <Eticheta iconita="lista">Rețeaua panoului</Eticheta>
          <T>
            {stare.retea.ap ? 'Rețeaua proprie NEPHA-casa' : stare.retea.nume || 'Wi-Fi'} · {stare.retea.ip}
          </T>
        </Card>
      ) : null}

      <T fel="mic" style={{ textAlign: 'center', marginTop: 8 }}>
        NEPHA · casa care se gândește singură cum să economisească energie
      </T>
    </Ecran>
  );
}

const stil = StyleSheet.create({
  camp: {
    borderWidth: 1,
    borderColor: culori.linie,
    borderRadius: colt.mic + 2,
    paddingHorizontal: 12,
    paddingVertical: 10,
    marginVertical: 12,
    fontFamily: fonturi.mediu,
    fontSize: 16,
    color: culori.text,
    backgroundColor: culori.card,
  },
  rand: { flexDirection: 'row', alignItems: 'center', gap: 10 },
  segment: { flexDirection: 'row', backgroundColor: culori.moale, borderRadius: 99, padding: 4 },
  segmentButon: { flex: 1, paddingVertical: 8, borderRadius: 99, alignItems: 'center' },
  segmentActiv: { backgroundColor: culori.card, shadowColor: '#000', shadowOpacity: 0.06, shadowRadius: 2, shadowOffset: { width: 0, height: 1 }, elevation: 1 },
  segmentText: { fontFamily: fonturi.gros, color: culori.text2 },
  pas: { paddingHorizontal: 14, paddingVertical: 6 },
  grade: { fontFamily: fonturi.gros, fontSize: 17, minWidth: 48, textAlign: 'center' },
});
