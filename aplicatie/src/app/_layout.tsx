// radacina aplicatiei: fontul Nunito (acelasi ca pe panou), starea casei si filele de jos
import { Nunito_400Regular, Nunito_600SemiBold, Nunito_700Bold, Nunito_800ExtraBold, useFonts } from '@expo-google-fonts/nunito';
import * as SplashScreen from 'expo-splash-screen';
import { StatusBar } from 'expo-status-bar';
import { useEffect } from 'react';
import { View } from 'react-native';

import File from '@/componente/file';
import { MesajJos } from '@/componente/ui';
import { CasaProvider } from '@/date/casa';
import { culori } from '@/tema';

SplashScreen.preventAutoHideAsync();

export default function Radacina() {
  const [fonturiGata, eroare] = useFonts({ Nunito_400Regular, Nunito_600SemiBold, Nunito_700Bold, Nunito_800ExtraBold });

  useEffect(() => {
    if (fonturiGata || eroare) SplashScreen.hideAsync();
  }, [fonturiGata, eroare]);

  if (!fonturiGata && !eroare) return null;

  return (
    <CasaProvider>
      <StatusBar style="dark" />
      <View style={{ flex: 1, backgroundColor: culori.fundal }}>
        <File />
        <MesajJos />
      </View>
    </CasaProvider>
  );
}
