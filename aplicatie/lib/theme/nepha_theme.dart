// tema NEPHA - aceleasi culori, colturi si font ca pagina casei si ecranul panoului
import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

// doua seturi de culori: ziua (alb cald, ca pagina casei) si noaptea (maro-negru cald, nu negru
// de tot, ca sa nu doara ochii). Culori.intunecat il schimba main.dart; restul aplicatiei
// citeste doar Culori.verde, Culori.text...
class _Set {
  final Color fundal, card, linie, moale, text, text2;
  final Color verde, verdeD, soare, soareD, apa, apaD, cald, caldD, rosu, rosuD;
  final Color cardPericol, umbra, mesaj;
  const _Set({
    required this.fundal,
    required this.card,
    required this.linie,
    required this.moale,
    required this.text,
    required this.text2,
    required this.verde,
    required this.verdeD,
    required this.soare,
    required this.soareD,
    required this.apa,
    required this.apaD,
    required this.cald,
    required this.caldD,
    required this.rosu,
    required this.rosuD,
    required this.cardPericol,
    required this.umbra,
    required this.mesaj,
  });
}

const _zi = _Set(
  fundal: Color(0xFFF7F4EF), // alb cald
  card: Color(0xFFFFFFFF),
  linie: Color(0xFFE8E3DA),
  moale: Color(0xFFEFEAE2), // fundalul bucatilor din carduri
  text: Color(0xFF2B2A28),
  text2: Color(0xFF8A857C),
  verde: Color(0xFF3F8F5E),
  verdeD: Color(0xFFE3F0E7),
  soare: Color(0xFFDE9428),
  soareD: Color(0xFFFBEBD0),
  apa: Color(0xFF3D86BD),
  apaD: Color(0xFFDDEBF6),
  cald: Color(0xFFD7684B),
  caldD: Color(0xFFF9E1DA),
  rosu: Color(0xFFCF3F3F),
  rosuD: Color(0xFFF8DADA),
  cardPericol: Color(0xFFFFF8F8),
  umbra: Color(0x0F2B2A28),
  mesaj: Color(0xFF26231F),
);

const _noapte = _Set(
  fundal: Color(0xFF171512),
  card: Color(0xFF221F1B),
  linie: Color(0xFF38332C),
  moale: Color(0xFF2D2924),
  text: Color(0xFFEDE8E0),
  text2: Color(0xFF9C968C),
  verde: Color(
    0xFF4E9E6B,
  ), // putin mai deschis, sa se vada pe negru (si scrisul alb pe el)
  verdeD: Color(0xFF1F3327),
  soare: Color(0xFFE9A640),
  soareD: Color(0xFF3A2D17),
  apa: Color(0xFF5FA3D6),
  apaD: Color(0xFF1B2C3A),
  cald: Color(0xFFE07E62),
  caldD: Color(0xFF3B231B),
  rosu: Color(0xFFE35D5D),
  rosuD: Color(0xFF3D1F1F),
  cardPericol: Color(0xFF2E1D1C),
  umbra: Color(0x33000000),
  mesaj: Color(0xFF3A3530),
);

class Culori {
  static bool intunecat = false;
  static _Set get _c => intunecat ? _noapte : _zi;

  static Color get fundal => _c.fundal;
  static Color get card => _c.card;
  static Color get linie => _c.linie;
  static Color get moale => _c.moale;
  static Color get text => _c.text;
  static Color get text2 => _c.text2;
  static Color get verde => _c.verde;
  static Color get verdeD => _c.verdeD;
  static Color get soare => _c.soare;
  static Color get soareD => _c.soareD;
  static Color get apa => _c.apa;
  static Color get apaD => _c.apaD;
  static Color get cald => _c.cald;
  static Color get caldD => _c.caldD;
  static Color get rosu => _c.rosu;
  static Color get rosuD => _c.rosuD;
  static Color get cardPericol => _c.cardPericol; // cardul alarmei cand suna
}

// aceeasi curba ca pe pagina si pe panou: porneste hotarat, se opreste bland
const usor = Cubic(0.4, 0, 0.2, 1);

const coltCard = 14.0;

// umbra fina de sub carduri
List<BoxShadow> get umbraCard => [
  BoxShadow(color: Culori._c.umbra, blurRadius: 2, offset: const Offset(0, 1)),
];

ThemeData temaNepha() {
  // fontul e in aplicatie (assets/google_fonts): nu-l mai cautam pe internet
  GoogleFonts.config.allowRuntimeFetching = false;

  final text = GoogleFonts.nunitoTextTheme().apply(
    bodyColor: Culori.text,
    displayColor: Culori.text,
  );
  return ThemeData(
    useMaterial3: true,
    brightness: Culori.intunecat ? Brightness.dark : Brightness.light,
    scaffoldBackgroundColor: Culori.fundal,
    colorScheme: ColorScheme.fromSeed(
      seedColor: Culori.verde,
      brightness: Culori.intunecat ? Brightness.dark : Brightness.light,
      primary: Culori.verde,
      surface: Culori.fundal,
      error: Culori.rosu,
    ),
    textTheme: text,
    cardTheme: CardThemeData(
      color: Culori.card,
      elevation: 0,
      margin: EdgeInsets.zero,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(coltCard),
        side: BorderSide(color: Culori.linie, width: 0.6),
      ),
    ),
    navigationBarTheme: NavigationBarThemeData(
      backgroundColor: Culori.card,
      indicatorColor: Culori.verdeD,
      surfaceTintColor: Colors.transparent,
      labelTextStyle: WidgetStateProperty.resolveWith(
        (s) => GoogleFonts.nunito(
          fontSize: 12,
          fontWeight: FontWeight.w700,
          color: s.contains(WidgetState.selected) ? Culori.verde : Culori.text2,
        ),
      ),
      iconTheme: WidgetStateProperty.resolveWith(
        (s) => IconThemeData(
          color: s.contains(WidgetState.selected) ? Culori.verde : Culori.text2,
        ),
      ),
    ),
    snackBarTheme: SnackBarThemeData(
      behavior: SnackBarBehavior.floating,
      backgroundColor: Culori._c.mesaj,
      contentTextStyle: GoogleFonts.nunito(color: Colors.white, fontSize: 14),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(99)),
    ),
  );
}
