// file - bara de jos a telefonului (cea nativa: pe iPhone si pe Android arata ca la sistem)
import { NativeTabs } from 'expo-router/native-tabs';

import { culori } from '@/tema';

export default function File() {
  return (
    <NativeTabs
      backgroundColor={culori.card}
      tintColor={culori.verde}
      indicatorColor={culori.verdeD}
      iconColor={{ default: culori.text2, selected: culori.verde }}
      labelStyle={{ default: { color: culori.text2 }, selected: { color: culori.verde } }}>
      <NativeTabs.Trigger name="index">
        <NativeTabs.Trigger.Label>Acasă</NativeTabs.Trigger.Label>
        <NativeTabs.Trigger.Icon sf={{ default: 'house', selected: 'house.fill' }} md="home" />
      </NativeTabs.Trigger>
      <NativeTabs.Trigger name="camere">
        <NativeTabs.Trigger.Label>Camere</NativeTabs.Trigger.Label>
        <NativeTabs.Trigger.Icon sf={{ default: 'lightbulb', selected: 'lightbulb.fill' }} md="lightbulb" />
      </NativeTabs.Trigger>
      <NativeTabs.Trigger name="energie">
        <NativeTabs.Trigger.Label>Energie</NativeTabs.Trigger.Label>
        <NativeTabs.Trigger.Icon sf={{ default: 'bolt', selected: 'bolt.fill' }} md="bolt" />
      </NativeTabs.Trigger>
      <NativeTabs.Trigger name="alarma">
        <NativeTabs.Trigger.Label>Alarmă</NativeTabs.Trigger.Label>
        <NativeTabs.Trigger.Icon sf={{ default: 'shield', selected: 'shield.fill' }} md="shield" />
      </NativeTabs.Trigger>
      <NativeTabs.Trigger name="setari">
        <NativeTabs.Trigger.Label>Setări</NativeTabs.Trigger.Label>
        <NativeTabs.Trigger.Icon sf={{ default: 'gearshape', selected: 'gearshape.fill' }} md="settings" />
      </NativeTabs.Trigger>
    </NativeTabs>
  );
}
