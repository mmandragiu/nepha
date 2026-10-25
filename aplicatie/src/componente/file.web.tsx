// file (web) - in browser nu exista bara nativa: desenam una asemanatoare, jos
import { TabList, TabSlot, Tabs, TabTrigger, type TabTriggerSlotProps } from 'expo-router/ui';
import { Pressable, StyleSheet, View } from 'react-native';

import { culori } from '@/tema';
import { Iconita, type NumeIconita } from './iconite';
import { T } from './ui';

type Fila = TabTriggerSlotProps & { iconita: NumeIconita; eticheta: string };

function Buton({ iconita, eticheta, isFocused, ref: _ref, ...rest }: Fila) {
  const c = isFocused ? culori.verde : culori.text2;
  return (
    <Pressable {...rest} style={stil.fila}>
      <View style={[stil.pastila, isFocused && { backgroundColor: culori.verdeD }]}>
        <Iconita nume={iconita} culoare={c} marime={20} />
      </View>
      <T style={[stil.eticheta, { color: c }]}>{eticheta}</T>
    </Pressable>
  );
}

export default function File() {
  return (
    <Tabs style={{ flex: 1 }}>
      <TabSlot style={{ flex: 1 }} />
      <TabList style={stil.bara}>
        <TabTrigger name="index" href="/" asChild>
          <Buton iconita="casa" eticheta="Acasă" />
        </TabTrigger>
        <TabTrigger name="camere" href="/camere" asChild>
          <Buton iconita="bec" eticheta="Camere" />
        </TabTrigger>
        <TabTrigger name="energie" href="/energie" asChild>
          <Buton iconita="fulger" eticheta="Energie" />
        </TabTrigger>
        <TabTrigger name="alarma" href="/alarma" asChild>
          <Buton iconita="scut" eticheta="Alarmă" />
        </TabTrigger>
        <TabTrigger name="setari" href="/setari" asChild>
          <Buton iconita="grila" eticheta="Setări" />
        </TabTrigger>
      </TabList>
    </Tabs>
  );
}

const stil = StyleSheet.create({
  bara: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    backgroundColor: culori.card,
    borderTopWidth: StyleSheet.hairlineWidth,
    borderTopColor: culori.linie,
    paddingTop: 6,
    paddingBottom: 10,
  },
  fila: { flex: 1, alignItems: 'center', gap: 2 },
  pastila: { paddingVertical: 4, paddingHorizontal: 18, borderRadius: 99 },
  eticheta: { fontSize: 12, fontFamily: 'Nunito_700Bold' },
});
