// ui - bucatile din care facem ecranele (acelasi aspect ca pagina casei)
import * as Haptics from 'expo-haptics';
import { useEffect, useState, type ReactNode } from 'react';
import {
  ActivityIndicator,
  Animated,
  Platform,
  Pressable,
  ScrollView,
  StyleSheet,
  Text,
  View,
  type StyleProp,
  type TextProps,
  type TextStyle,
  type ViewStyle,
} from 'react-native';
import { useSafeAreaInsets } from 'react-native-safe-area-context';

import { useCasa, type Conexiune } from '@/date/casa';
import { colt, culori, fonturi, umbra, usor } from '@/tema';
import { Iconita, type NumeIconita } from './iconite';

// o valoare pentru animatii, facuta o singura data (useAnimatedValue nu exista pe web)
function useAnimatedValue(initial: number): Animated.Value {
  const [v] = useState(() => new Animated.Value(initial));
  return v;
}

// o vibratie scurta la atingere (pe web nu exista)
export function atingere() {
  if (Platform.OS !== 'web') Haptics.selectionAsync().catch(() => {});
}

// ---------------------------------------------------------------- text

type Fel = 'normal' | 'mic' | 'eticheta' | 'titlu' | 'mediu' | 'mare';

export function T({ fel = 'normal', style, ...rest }: TextProps & { fel?: Fel }) {
  return <Text {...rest} style={[stil.text, stil[fel], style]} />;
}

// ---------------------------------------------------------------- ecran

export function Ecran({ titlu, subtitlu, dreapta, children }: { titlu: string; subtitlu?: string; dreapta?: ReactNode; children: ReactNode }) {
  const margini = useSafeAreaInsets();
  return (
    <ScrollView
      style={stil.ecran}
      contentInsetAdjustmentBehavior="automatic"
      contentContainerStyle={[
        stil.continut,
        // pe iOS bara de sus o lasa ScrollView-ul; pe Android si pe web o lasam noi
        Platform.OS !== 'ios' && { paddingTop: margini.top + 16 },
      ]}>
      <View style={stil.antet}>
        <View style={{ flex: 1 }}>
          <T fel="titlu">{titlu}</T>
          {subtitlu ? <T fel="mic">{subtitlu}</T> : null}
        </View>
        {dreapta}
      </View>
      <BandaDemo />
      {children}
    </ScrollView>
  );
}

function BandaDemo() {
  const { conexiune } = useCasa();
  if (conexiune !== 'demo') return null;
  return (
    <View style={stil.banda}>
      <T fel="mic" style={{ color: culori.text }}>
        Mod demo: vedeți o seară simulată. Adresa panoului se pune în Setări.
      </T>
    </View>
  );
}

// ---------------------------------------------------------------- card

export function Card({ children, style, onPress }: { children: ReactNode; style?: StyleProp<ViewStyle>; onPress?: () => void }) {
  if (!onPress) return <View style={[stil.card, style]}>{children}</View>;
  return (
    <Pressable
      onPress={() => {
        atingere();
        onPress();
      }}
      style={({ pressed }) => [stil.card, style, pressed && stil.cardApasat]}>
      {children}
    </Pressable>
  );
}

export function Eticheta({ iconita, culoare = culori.text2, children, dreapta }: { iconita?: NumeIconita; culoare?: string; children: ReactNode; dreapta?: ReactNode }) {
  return (
    <View style={stil.etichetaRand}>
      {iconita ? <Iconita nume={iconita} culoare={culoare} /> : null}
      <T fel="eticheta" style={{ flex: 1 }} numberOfLines={1}>
        {children}
      </T>
      {dreapta}
    </View>
  );
}

// ---------------------------------------------------------------- pastila

type Ton = 'gri' | 'verde' | 'soare' | 'apa' | 'cald' | 'rosu';
const TONURI: Record<Ton, [string, string]> = {
  gri: [culori.moale, culori.text2],
  verde: [culori.verdeD, culori.verde],
  soare: [culori.soareD, culori.soare],
  apa: [culori.apaD, culori.apa],
  cald: [culori.caldD, culori.cald],
  rosu: [culori.rosuD, culori.rosu],
};

export function Pastila({ ton = 'gri', children, onPress, asteapta }: { ton?: Ton; children: ReactNode; onPress?: () => void; asteapta?: boolean }) {
  const [fundal, text] = TONURI[ton];
  const continut = (
    <>
      {asteapta ? <ActivityIndicator size="small" color={text} style={{ transform: [{ scale: 0.6 }] }} /> : null}
      <T style={[stil.pastilaText, { color: text }]}>{children}</T>
    </>
  );
  if (!onPress) return <View style={[stil.pastila, { backgroundColor: fundal }]}>{continut}</View>;
  return (
    <Pressable
      hitSlop={8}
      onPress={() => {
        atingere();
        onPress();
      }}
      style={({ pressed }) => [stil.pastila, { backgroundColor: fundal }, pressed && { opacity: 0.7 }]}>
      {continut}
    </Pressable>
  );
}

// ---------------------------------------------------------------- buton

export function Buton({ children, onPress, principal, dezactivat, style }: { children: ReactNode; onPress: () => void; principal?: boolean; dezactivat?: boolean; style?: StyleProp<ViewStyle> }) {
  return (
    <Pressable
      disabled={dezactivat}
      onPress={() => {
        atingere();
        onPress();
      }}
      style={({ pressed }) => [
        stil.buton,
        principal && stil.butonPrincipal,
        pressed && { transform: [{ translateY: 1 }], opacity: 0.85 },
        dezactivat && { opacity: 0.5 },
        style,
      ]}>
      <T style={[stil.butonText, principal && { color: '#fff' }]}>{children}</T>
    </Pressable>
  );
}

// ---------------------------------------------------------------- comutator

// se muta pe loc; cat asteptam raspunsul panoului, arata un cerc care se invarte
export function Comutator({ pornit, onSchimba, asteapta, culoare = culori.soare, eticheta }: { pornit: boolean; onSchimba: (nou: boolean) => void; asteapta?: boolean; culoare?: string; eticheta?: string }) {
  const poz = useAnimatedValue(pornit ? 1 : 0);
  useEffect(() => {
    Animated.timing(poz, { toValue: pornit ? 1 : 0, duration: 200, easing: usor, useNativeDriver: false }).start();
  }, [pornit, poz]);
  const fundal = poz.interpolate({ inputRange: [0, 1], outputRange: [culori.linie, culoare] });
  const x = poz.interpolate({ inputRange: [0, 1], outputRange: [3, 21] });
  return (
    <Pressable
      accessibilityRole="switch"
      accessibilityState={{ checked: pornit }}
      accessibilityLabel={eticheta}
      hitSlop={8}
      onPress={() => {
        atingere();
        onSchimba(!pornit);
      }}>
      <Animated.View style={[stil.comutator, { backgroundColor: fundal }]}>
        <Animated.View style={[stil.buton_comutator, { transform: [{ translateX: x }] }]}>
          {asteapta ? <ActivityIndicator size="small" color={culori.text2} style={{ transform: [{ scale: 0.55 }] }} /> : null}
        </Animated.View>
      </Animated.View>
    </Pressable>
  );
}

// ---------------------------------------------------------------- bara

// bara aluneca in 0,5 s, cu aceeasi curba ca pe panou
export function Bara({ procent, culoare = culori.verde, inaltime = 8, fundal = culori.linie }: { procent: number; culoare?: string; inaltime?: number; fundal?: string }) {
  const [latime, setLatime] = useState(0);
  const p = useAnimatedValue(0);
  useEffect(() => {
    Animated.timing(p, { toValue: Math.max(0, Math.min(100, procent)), duration: 500, easing: usor, useNativeDriver: false }).start();
  }, [procent, p]);
  return (
    <View onLayout={(e) => setLatime(e.nativeEvent.layout.width)} style={{ height: inaltime, borderRadius: 99, backgroundColor: fundal, overflow: 'hidden' }}>
      <Animated.View
        style={{
          height: '100%',
          borderRadius: 99,
          backgroundColor: culoare,
          width: p.interpolate({ inputRange: [0, 100], outputRange: [0, latime] }),
        }}
      />
    </View>
  );
}

// ---------------------------------------------------------------- punct care pulseaza

export function Punct({ culoare, pulseaza }: { culoare: string; pulseaza?: boolean }) {
  const a = useAnimatedValue(0);
  useEffect(() => {
    if (!pulseaza) return;
    const bucla = Animated.loop(Animated.timing(a, { toValue: 1, duration: 2000, easing: usor, useNativeDriver: true }));
    bucla.start();
    return () => bucla.stop();
  }, [pulseaza, a]);
  return (
    <View style={{ width: 8, height: 8, alignItems: 'center', justifyContent: 'center' }}>
      {pulseaza ? (
        <Animated.View
          style={{
            position: 'absolute',
            width: 8,
            height: 8,
            borderRadius: 4,
            backgroundColor: culoare,
            opacity: a.interpolate({ inputRange: [0, 0.7, 1], outputRange: [0.45, 0, 0] }),
            transform: [{ scale: a.interpolate({ inputRange: [0, 0.7, 1], outputRange: [1, 2.4, 2.4] }) }],
          }}
        />
      ) : null}
      <View style={{ width: 8, height: 8, borderRadius: 4, backgroundColor: culoare }} />
    </View>
  );
}

// ---------------------------------------------------------------- legatura cu panoul

const CONEXIUNE: Record<Conexiune, [string, string, boolean]> = {
  demo: ['demo', culori.soare, false],
  'fara-adresa': ['fără panou', culori.text2, false],
  'se-conecteaza': ['se conectează...', culori.soare, true],
  conectat: ['conectat la casă', culori.verde, true],
  'arduino-tace': ['Arduino nu răspunde', culori.soare, false],
  deconectat: ['panoul nu răspunde', culori.rosu, false],
};

export function IndicatorConexiune() {
  const { conexiune } = useCasa();
  const [text, culoare, pulseaza] = CONEXIUNE[conexiune];
  return (
    <View style={stil.indicator}>
      <Punct culoare={culoare} pulseaza={pulseaza} />
      <T fel="mic">{text}</T>
    </View>
  );
}

// ---------------------------------------------------------------- mesaj jos pe ecran

export function MesajJos() {
  const { mesaj } = useCasa();
  const margini = useSafeAreaInsets();
  const y = useAnimatedValue(0);
  const vizibil = !!mesaj?.vizibil;
  useEffect(() => {
    Animated.timing(y, { toValue: vizibil ? 1 : 0, duration: 300, easing: usor, useNativeDriver: true }).start();
  }, [vizibil, y]);
  return (
    <Animated.View
      pointerEvents="none"
      style={[
        stil.mesaj,
        { bottom: margini.bottom + 96, opacity: y, transform: [{ translateY: y.interpolate({ inputRange: [0, 1], outputRange: [20, 0] }) }] },
        mesaj?.ton === 'eroare' && { backgroundColor: culori.rosu },
      ]}>
      <T style={{ color: '#fff' }}>{mesaj?.text ?? ''}</T>
    </Animated.View>
  );
}

// ---------------------------------------------------------------- stiluri

const stil = StyleSheet.create({
  text: { fontFamily: fonturi.normal, fontSize: 15, color: culori.text },
  normal: {},
  mic: { fontSize: 13, color: culori.text2 },
  eticheta: { fontFamily: fonturi.gros, fontSize: 14, color: culori.text2 },
  titlu: { fontFamily: fonturi.foarteGros, fontSize: 28, letterSpacing: -0.5 },
  mediu: { fontFamily: fonturi.gros, fontSize: 24, letterSpacing: -0.3 },
  mare: { fontFamily: fonturi.foarteGros, fontSize: 44, letterSpacing: -1, lineHeight: 50 },
  ecran: { flex: 1, backgroundColor: culori.fundal },
  continut: { padding: 16, paddingBottom: 40, gap: 12 },
  antet: { flexDirection: 'row', alignItems: 'center', gap: 12, marginBottom: 4 },
  banda: { backgroundColor: culori.soareD, borderRadius: colt.card, paddingVertical: 10, paddingHorizontal: 14 },
  card: {
    backgroundColor: culori.card,
    borderRadius: colt.card,
    borderWidth: StyleSheet.hairlineWidth,
    borderColor: culori.linie,
    padding: 16,
    ...umbra,
  },
  cardApasat: { transform: [{ translateY: -2 }], opacity: 0.96 },
  etichetaRand: { flexDirection: 'row', alignItems: 'center', gap: 8, minHeight: 26, marginBottom: 10 },
  pastila: { flexDirection: 'row', alignItems: 'center', gap: 2, paddingVertical: 4, paddingHorizontal: 10, borderRadius: 99, alignSelf: 'flex-start' },
  pastilaText: { fontFamily: fonturi.gros, fontSize: 12 } as TextStyle,
  buton: {
    borderWidth: 1,
    borderColor: culori.linie,
    backgroundColor: culori.card,
    borderRadius: colt.buton,
    paddingVertical: 11,
    paddingHorizontal: 18,
    alignItems: 'center',
  },
  butonPrincipal: { backgroundColor: culori.verde, borderColor: culori.verde },
  butonText: { fontFamily: fonturi.gros, fontSize: 15, color: culori.text },
  comutator: { width: 46, height: 28, borderRadius: 99, justifyContent: 'center' },
  buton_comutator: {
    width: 22,
    height: 22,
    borderRadius: 11,
    backgroundColor: culori.card,
    alignItems: 'center',
    justifyContent: 'center',
    shadowColor: '#000',
    shadowOpacity: 0.2,
    shadowRadius: 2,
    shadowOffset: { width: 0, height: 1 },
    elevation: 2,
  },
  indicator: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
    paddingVertical: 6,
    paddingHorizontal: 12,
    borderRadius: 99,
    backgroundColor: culori.card,
    borderWidth: StyleSheet.hairlineWidth,
    borderColor: culori.linie,
  },
  mesaj: {
    position: 'absolute',
    alignSelf: 'center',
    maxWidth: '90%',
    backgroundColor: '#26231F',
    paddingVertical: 10,
    paddingHorizontal: 16,
    borderRadius: 99,
  },
});
