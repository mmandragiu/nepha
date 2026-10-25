// Camere - cine e unde, temperatura, lumina (se aprinde de aici) si ce a invatat casa
import { StyleSheet, View } from 'react-native';

import { HartaRutina } from '@/componente/grafice';
import { Card, Comutator, Ecran, Eticheta, IndicatorConexiune, Pastila, T } from '@/componente/ui';
import { useCasa } from '@/date/casa';
import { grade } from '@/date/texte';
import { culori } from '@/tema';

export default function Camere() {
  const { stare, rutina, comanda, arata, asteapta } = useCasa();
  const zile = rutina ? Math.max(...rutina.zile) : 0;

  return (
    <Ecran titlu="Camere" subtitlu="atingeți comutatorul ca să aprindeți becul" dreapta={<IndicatorConexiune />}>
      {(stare?.camere ?? []).map((c, i) => {
        const cheie = `lumina:${i}`;
        const aprinsa = arata(cheie, c.lumina > 0);
        return (
          <Card key={c.nume}>
            <View style={stil.rand}>
              <View style={{ flex: 1, gap: 4 }}>
                <T style={stil.nume}>{c.nume}</T>
                <T fel="mic">
                  {c.prezenta ? 'e cineva' : 'liber'}
                  {c.lumina ? ` · lumină ${c.lumina}%` : ''}
                </T>
                {c.fereastra ? <Pastila ton="cald">geam deschis</Pastila> : null}
              </View>
              <T fel="mediu">{grade(c.temp)}</T>
              <Comutator
                eticheta={`Lumina din ${c.nume}`}
                pornit={aprinsa}
                asteapta={asteapta(cheie)}
                onSchimba={(nou) => comanda(`LUMINA ${i} ${nou ? 100 : 0}`, { cheie, valoare: nou })}
              />
            </View>
          </Card>
        );
      })}

      <Card>
        <Eticheta iconita="grila" culoare={culori.verde} dreapta={rutina?.valida ? <T fel="mic">din {zile} {zile === 1 ? 'zi' : 'zile'}</T> : null}>
          Ce a învățat casa
        </Eticheta>
        <HartaRutina rutina={rutina} oraAcum={stare ? Math.floor(stare.minut / 60) : -1} />
        <T fel="mic" style={{ marginTop: 12 }}>
          Cât de probabil e să fie cineva în fiecare cameră, la fiecare oră. Din asta casa știe când să pornească
          încălzirea mai devreme și când să stingă lumina mai repede.
        </T>
      </Card>
    </Ecran>
  );
}

const stil = StyleSheet.create({
  rand: { flexDirection: 'row', alignItems: 'center', gap: 14 },
  nume: { fontFamily: 'Nunito_700Bold', fontSize: 17 },
});
