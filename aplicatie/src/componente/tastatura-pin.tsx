// tastatura-pin - PIN-ul alarmei. Il verifica Arduino, nu aplicatia: noi doar il trimitem.
import { useState } from 'react';
import { Modal, Pressable, StyleSheet, View } from 'react-native';

import { colt, culori, fonturi } from '@/tema';
import { atingere, Buton, T } from './ui';

const TASTE = ['1', '2', '3', '4', '5', '6', '7', '8', '9', '', '0', '⌫'];

export function TastaturaPin({ vizibil, onInchide, onTrimite }: { vizibil: boolean; onInchide: () => void; onTrimite: (pin: string) => void }) {
  const [pin, setPin] = useState('');
  const inchide = () => {
    setPin('');
    onInchide();
  };

  const apasa = (k: string) => {
    atingere();
    if (k === '⌫') setPin((p) => p.slice(0, -1));
    else if (k && pin.length < 4) setPin((p) => p + k);
  };

  return (
    <Modal visible={vizibil} transparent animationType="fade" onRequestClose={inchide}>
      <Pressable style={stil.fundal} onPress={inchide}>
        <Pressable style={stil.fereastra} onPress={() => {}}>
          <T fel="mediu">PIN-ul alarmei</T>
          <T fel="mic">Îl verifică Arduino, nu aplicația.</T>
          <View style={stil.puncte}>
            {[0, 1, 2, 3].map((i) => (
              <View key={i} style={[stil.punct, i < pin.length && stil.punctPlin]} />
            ))}
          </View>
          <View style={stil.taste}>
            {TASTE.map((k, i) =>
              k ? (
                <Pressable key={i} onPress={() => apasa(k)} style={({ pressed }) => [stil.tasta, pressed && { backgroundColor: culori.linie }]}>
                  <T style={stil.tastaText}>{k}</T>
                </Pressable>
              ) : (
                <View key={i} style={stil.tastaGoala} />
              ),
            )}
          </View>
          <View style={stil.butoane}>
            <Buton onPress={inchide} style={{ flex: 1 }}>
              Renunț
            </Buton>
            <Buton principal dezactivat={pin.length === 0} onPress={() => {
                setPin('');
                onTrimite(pin);
              }} style={{ flex: 1 }}>
              Dezarmează
            </Buton>
          </View>
        </Pressable>
      </Pressable>
    </Modal>
  );
}

const stil = StyleSheet.create({
  fundal: { flex: 1, backgroundColor: 'rgba(23,21,18,0.45)', alignItems: 'center', justifyContent: 'center', padding: 16 },
  fereastra: { width: '100%', maxWidth: 340, backgroundColor: culori.card, borderRadius: 16, padding: 20, gap: 4 },
  puncte: { flexDirection: 'row', justifyContent: 'center', gap: 12, marginVertical: 14 },
  punct: { width: 14, height: 14, borderRadius: 7, borderWidth: 2, borderColor: culori.linie },
  punctPlin: { backgroundColor: culori.text, borderColor: culori.text },
  taste: { flexDirection: 'row', flexWrap: 'wrap', gap: 8, justifyContent: 'space-between' },
  tasta: { width: '31%', paddingVertical: 14, borderRadius: colt.mic + 2, backgroundColor: culori.moale, alignItems: 'center' },
  tastaGoala: { width: '31%' },
  tastaText: { fontFamily: fonturi.gros, fontSize: 20 },
  butoane: { flexDirection: 'row', gap: 8, marginTop: 16 },
});
