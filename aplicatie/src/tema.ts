// tema - aceleasi culori ca pagina casei (web/index.html) si ca ecranul panoului
import { Easing, Platform } from 'react-native';

export const culori = {
  fundal: '#F7F4EF', // alb cald
  card: '#FFFFFF',
  linie: '#E8E3DA',
  moale: '#EFEAE2', // fundalul bucatilor din carduri
  text: '#2B2A28',
  text2: '#8A857C',
  verde: '#3F8F5E',
  verdeD: '#E3F0E7',
  soare: '#DE9428',
  soareD: '#FBEBD0',
  apa: '#3D86BD',
  apaD: '#DDEBF6',
  cald: '#D7684B',
  caldD: '#F9E1DA',
  rosu: '#CF3F3F',
  rosuD: '#F8DADA',
} as const;

export const fonturi = {
  normal: 'Nunito_400Regular',
  mediu: 'Nunito_600SemiBold',
  gros: 'Nunito_700Bold',
  foarteGros: 'Nunito_800ExtraBold',
} as const;

export const colt = { card: 14, buton: 999, mic: 10 } as const;

// umbra fina de sub carduri (pe Android se vede doar elevation)
export const umbra = Platform.select({
  android: { elevation: 1 },
  default: {
    shadowColor: '#2B2A28',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.06,
    shadowRadius: 2,
  },
});

// aceeasi curba ca pe pagina si pe panou: porneste hotarat, se opreste bland
export const usor = Easing.bezier(0.4, 0, 0.2, 1);
