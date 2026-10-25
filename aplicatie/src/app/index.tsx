// Acasa - pe scurt: cat am economisit azi, cum e in casa si afara, alarma, gradina, ce a decis casa
import { router } from 'expo-router';
import { useState } from 'react';
import { StyleSheet, View } from 'react-native';

import { Inel } from '@/componente/grafice';
import { Bara, Buton, Card, Ecran, Eticheta, IndicatorConexiune, Pastila, Punct, T } from '@/componente/ui';
import { useCasa } from '@/date/casa';
import { grade, kwhEconomisiti, nr, salut, tempCasa, textVreme } from '@/date/texte';
import { culori } from '@/tema';

export default function Acasa() {
  const { stare, jurnal, conexiune, comanda } = useCasa();
  const [totJurnalul, setTotJurnalul] = useState(false);

  if (!stare) {
    return (
      <Ecran titlu="NEPHA" dreapta={<IndicatorConexiune />}>
        <Card>
          <T>
            {conexiune === 'fara-adresa'
              ? 'Aplicația nu știe încă unde e panoul. Puneți-i adresa în Setări (o vedeți pe ecranul panoului), sau porniți modul demo.'
              : conexiune === 'deconectat'
                ? 'Panoul nu răspunde. Telefonul trebuie să fie în aceeași rețea Wi-Fi cu casa (sau în rețeaua NEPHA-casa).'
                : 'Se conectează la casă...'}
          </T>
          {conexiune === 'fara-adresa' || conexiune === 'deconectat' ? (
            <Buton principal onPress={() => router.navigate('/setari')} style={{ marginTop: 14 }}>
              Deschide setările
            </Buton>
          ) : null}
        </Card>
      </Ecran>
    );
  }

  const smart = stare.mod === 'smart';
  const kwh = kwhEconomisiti(stare);
  const clasic = stare.energie.whClasic || 0;
  const noi = stare.energie.whAzi || 0;
  const inc = stare.incalzire;
  const al = stare.alarma;
  const pericol = al.cod >= 4;
  const deschise = al.senzori.filter((s) => s.deschis).length;
  const en = stare.energie;
  const culoareBaterie = en.modCod === 3 ? culori.rosu : en.modCod === 2 ? culori.soare : culori.verde;
  const decizii = totJurnalul ? jurnal : jurnal.slice(0, 3);

  return (
    <Ecran
      titlu={salut(stare.minut)}
      subtitlu={`Casa NEPHA · ${stare.ora}${stare.test === 1 ? ' · ziua de test rulează' : ''}`}
      dreapta={<IndicatorConexiune />}>
      {/* economia de azi */}
      <Card>
        <Eticheta
          iconita="frunza"
          culoare={culori.verde}
          dreapta={
            <View style={stil.mod}>
              <Punct culoare={smart ? culori.verde : culori.text2} pulseaza={smart} />
              <T fel="mic">{smart ? 'Smart' : 'Clasic'}</T>
            </View>
          }>
          Economie azi
        </Eticheta>
        {smart ? (
          <>
            <View style={stil.randMare}>
              <T fel="mare" style={{ color: culori.verde }}>
                {nr(kwh, 1)}
              </T>
              <T fel="mediu"> kWh</T>
              <T fel="mic" style={{ marginLeft: 'auto' }}>
                ≈ {nr(kwh * stare.pretKwh, 1)} lei
              </T>
            </View>
            <T fel="mic">față de o casă obișnuită, în aceleași condiții</T>
            <View style={stil.compara}>
              <RandCompara nume="Casă clasică" wh={clasic} procent={100} culoare={culori.text2} />
              <RandCompara nume="NEPHA" wh={noi} procent={clasic > 0 ? (noi / clasic) * 100 : 0} culoare={culori.verde} />
            </View>
          </>
        ) : (
          <T fel="mic">Casa e în modul clasic: merge ca o casă obișnuită, fără să economisească.</T>
        )}
      </Card>

      {/* in casa / afara */}
      <View style={stil.doua}>
        <Card style={stil.jumatate} onPress={() => router.navigate('/camere')}>
          <Eticheta iconita="termometru" culoare={culori.cald}>
            În casă
          </Eticheta>
          <T fel="mare">{grade(tempCasa(stare))}</T>
          <View style={{ marginTop: 8 }}>
            <Pastila ton={inc.pornita ? 'cald' : 'gri'}>{inc.pornita ? 'încălzește' : 'în repaus'}</Pastila>
          </View>
          <T fel="mic" style={{ marginTop: 6 }}>
            Țintă {grade(inc.tinta)} · {inc.motiv}
          </T>
        </Card>
        <Card style={stil.jumatate}>
          <Eticheta iconita="soare" culoare={culori.apa}>
            Afară
          </Eticheta>
          <T fel="mare">{grade(stare.afara ?? (stare.vreme.valida ? (stare.vreme.temp ?? null) : null))}</T>
          <T fel="mic" style={{ marginTop: 8 }}>
            {textVreme(stare)}
          </T>
        </Card>
      </View>

      {/* bateria */}
      <Card onPress={() => router.navigate('/energie')}>
        <Eticheta iconita="fulger" culoare={culori.soare} dreapta={<Pastila ton={en.modCod === 3 ? 'rosu' : en.modCod === 2 ? 'soare' : 'verde'}>{`Mod ${en.mod.toLowerCase()}`}</Pastila>}>
          Baterie
        </Eticheta>
        <View style={stil.randBaterie}>
          <T fel="mediu">{en.baterie}%</T>
          <View style={{ flex: 1 }}>
            <Bara procent={en.baterie} culoare={culoareBaterie} />
          </View>
        </View>
      </Card>

      {/* alarma */}
      <Card onPress={() => router.navigate('/alarma')} style={pericol && stil.pericol}>
        <Eticheta iconita="scut" culoare={pericol ? culori.rosu : al.cod === 0 ? culori.text2 : culori.verde} dreapta={<T fel="mic">›</T>}>
          Alarmă
        </Eticheta>
        <T fel="mediu" style={pericol && { color: culori.rosu }}>
          {al.stare}
        </T>
        <T fel="mic" style={{ marginTop: 4 }}>
          {deschise ? `${deschise} ${deschise === 1 ? 'ușă sau geam deschis' : 'uși sau geamuri deschise'}` : 'Toate ușile și geamurile sunt închise'}
        </T>
      </Card>

      {/* gradina */}
      <Card>
        <Eticheta iconita="picatura" culoare={culori.apa}>
          Grădină
        </Eticheta>
        <View style={stil.gradina}>
          <Inel procent={stare.gradina.umiditate} marime={110} />
          <View style={{ flex: 1, gap: 10 }}>
            <T fel="mic">
              {stare.gradina.pompa
                ? 'Se udă acum.'
                : stare.gradina.umiditate >= 35
                  ? 'Pământul e destul de umed.'
                  : 'Pământul e uscat: casa așteaptă momentul potrivit.'}
            </T>
            <Buton onPress={() => comanda('UDA')} dezactivat={stare.gradina.pompa}>
              Udă acum
            </Buton>
          </View>
        </View>
      </Card>

      {/* jurnalul */}
      <Card>
        <Eticheta iconita="lista" dreapta={<T fel="mic">ce a decis și de ce</T>}>
          Jurnalul casei
        </Eticheta>
        {decizii.length ? (
          decizii.map((e, i) => (
            <View key={e.ora + i} style={[stil.decizie, i > 0 && stil.cuLinie]}>
              <T style={stil.oraDecizie}>{e.ora}</T>
              <T style={{ flex: 1 }}>{e.text}</T>
            </View>
          ))
        ) : (
          <T fel="mic">Încă nimic. Aici apar deciziile casei, cu motivul lor.</T>
        )}
        {jurnal.length > 3 ? (
          <Buton onPress={() => setTotJurnalul((x) => !x)} style={{ marginTop: 12 }}>
            {totJurnalul ? 'Arată mai puțin' : `Arată tot (${jurnal.length})`}
          </Buton>
        ) : null}
      </Card>
    </Ecran>
  );
}

function RandCompara({ nume, wh, procent, culoare }: { nume: string; wh: number; procent: number; culoare: string }) {
  return (
    <View style={stil.randCompara}>
      <T fel="mic" style={{ width: 90, color: culori.text }}>
        {nume}
      </T>
      <View style={{ flex: 1 }}>
        <Bara procent={procent} culoare={culoare} fundal={culori.moale} />
      </View>
      <T fel="mic" style={{ width: 64, textAlign: 'right', color: culori.text }}>
        {nr(wh / 1000, 1)} kWh
      </T>
    </View>
  );
}

const stil = StyleSheet.create({
  mod: { flexDirection: 'row', alignItems: 'center', gap: 6 },
  randMare: { flexDirection: 'row', alignItems: 'baseline' },
  compara: { gap: 8, marginTop: 14 },
  randCompara: { flexDirection: 'row', alignItems: 'center', gap: 10 },
  doua: { flexDirection: 'row', gap: 12 },
  jumatate: { flex: 1 },
  randBaterie: { flexDirection: 'row', alignItems: 'center', gap: 14 },
  pericol: { borderColor: culori.rosu, borderWidth: 1.5, backgroundColor: '#FFF8F8' },
  gradina: { flexDirection: 'row', alignItems: 'center', gap: 16 },
  decizie: { flexDirection: 'row', gap: 10, paddingVertical: 9 },
  cuLinie: { borderTopWidth: StyleSheet.hairlineWidth, borderTopColor: culori.linie },
  oraDecizie: { width: 46, color: culori.verde, fontFamily: 'Nunito_700Bold' },
});
