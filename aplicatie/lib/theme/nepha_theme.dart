// tema NEPHA - curata, fara efecte: carduri plate cu colturi mari, un singur accent portocaliu.
// Ziua: gri deschis si carduri albe. Noaptea: grafit (nu negru de tot), carduri putin mai deschise.
// Sticla (blur, ca "Liquid Glass" de pe iPhone) o folosim doar pe ce pluteste peste ecran:
// bara de jos si butoanele de sus (vezi Sticla in widgets/ui.dart).
import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

class _Set {
  final Color fundal, card, moale, linie, text, text2;
  final Color accent, accentD; // portocaliu: ce e pornit, ce apasam
  final Color verde, verdeD; // economie, productie
  final Color soare, soareD, apa, apaD, rosu, rosuD;
  final Color cardPericol, umbra, mesaj;
  const _Set({
    required this.fundal,
    required this.card,
    required this.moale,
    required this.linie,
    required this.text,
    required this.text2,
    required this.accent,
    required this.accentD,
    required this.verde,
    required this.verdeD,
    required this.soare,
    required this.soareD,
    required this.apa,
    required this.apaD,
    required this.rosu,
    required this.rosuD,
    required this.cardPericol,
    required this.umbra,
    required this.mesaj,
  });
}

const _zi = _Set(
  fundal: Color(0xFFF1F1F3),
  card: Color(0xFFFFFFFF),
  moale: Color(0xFFF3F3F5), // bucatile din carduri, cercurile iconitelor
  linie: Color(0x12000000),
  text: Color(0xFF141416),
  text2: Color(0xFF8A8A93),
  accent: Color(0xFFF2612E),
  accentD: Color(0x1AF2612E),
  verde: Color(0xFF55A83A),
  verdeD: Color(0x1C55A83A),
  soare: Color(0xFFEFA21A),
  soareD: Color(0x1FEFA21A),
  apa: Color(0xFF3D8BEA),
  apaD: Color(0x1A3D8BEA),
  rosu: Color(0xFFE5484D),
  rosuD: Color(0x1AE5484D),
  cardPericol: Color(0xFFFFF4F4),
  umbra: Color(0x0D000000),
  mesaj: Color(0xFF1C1C1E),
);

const _noapte = _Set(
  fundal: Color(0xFF111112),
  card: Color(0xFF1C1C1E),
  moale: Color(0xFF2A2A2D),
  linie: Color(0x14FFFFFF),
  text: Color(0xFFF5F5F7),
  text2: Color(0xFF8E8E93),
  accent: Color(0xFFFF6A3D),
  accentD: Color(0x26FF6A3D),
  verde: Color(0xFF7CC85A),
  verdeD: Color(0x267CC85A),
  soare: Color(0xFFF5B53D),
  soareD: Color(0x26F5B53D),
  apa: Color(0xFF5AA2F5),
  apaD: Color(0x265AA2F5),
  rosu: Color(0xFFFF5A5F),
  rosuD: Color(0x29FF5A5F),
  cardPericol: Color(0xFF2B1B1C),
  umbra: Color(
    0x00000000,
  ), // noaptea cardurile se deosebesc prin culoare, nu prin umbra
  mesaj: Color(0xFF2C2C2E),
);

class Culori {
  static bool intunecat = false;
  static _Set get _c => intunecat ? _noapte : _zi;

  static Color get fundal => _c.fundal;
  static Color get card => _c.card;
  static Color get moale => _c.moale;
  static Color get linie => _c.linie;
  static Color get text => _c.text;
  static Color get text2 => _c.text2;
  static Color get accent => _c.accent;
  static Color get accentD => _c.accentD;
  static Color get verde => _c.verde;
  static Color get verdeD => _c.verdeD;
  static Color get soare => _c.soare;
  static Color get soareD => _c.soareD;
  static Color get apa => _c.apa;
  static Color get apaD => _c.apaD;
  // caldura si "atentie" au aceeasi culoare ca accentul (portocaliu)
  static Color get cald => _c.accent;
  static Color get caldD => _c.accentD;
  static Color get rosu => _c.rosu;
  static Color get rosuD => _c.rosuD;
  static Color get cardPericol => _c.cardPericol; // cardul alarmei cand suna
  static Color get mesaj => _c.mesaj;
  static const peAccent = Colors.white;
}

// aceeasi curba ca pe pagina si pe panou: porneste hotarat, se opreste bland
const usor = Cubic(0.4, 0, 0.2, 1);

const coltCard = 28.0;
const coltMic = 18.0;

// umbra foarte fina de sub carduri (doar ziua)
List<BoxShadow> get umbraCard => [
  if (!Culori.intunecat)
    BoxShadow(
      color: Culori._c.umbra,
      blurRadius: 24,
      offset: const Offset(0, 6),
      spreadRadius: -4,
    ),
];

TextStyle fontText({double? marime, FontWeight? gros, Color? culoare}) =>
    GoogleFonts.urbanist(fontSize: marime, fontWeight: gros, color: culoare);

// datele tehnice din "Date avansate" (adrese, protocoale, milisecunde)
TextStyle fontCod({double? marime, FontWeight? gros, Color? culoare}) =>
    GoogleFonts.jetBrainsMono(
      fontSize: marime,
      fontWeight: gros,
      color: culoare,
    );

ThemeData temaNepha() {
  // fonturile sunt in aplicatie (assets/google_fonts): nu le mai cautam pe internet
  GoogleFonts.config.allowRuntimeFetching = false;

  final text = GoogleFonts.urbanistTextTheme().apply(
    bodyColor: Culori.text,
    displayColor: Culori.text,
  );
  final noapte = Culori.intunecat;
  return ThemeData(
    useMaterial3: true,
    brightness: noapte ? Brightness.dark : Brightness.light,
    scaffoldBackgroundColor: Culori.fundal,
    colorScheme: ColorScheme.fromSeed(
      seedColor: Culori.accent,
      brightness: noapte ? Brightness.dark : Brightness.light,
      primary: Culori.accent,
      onPrimary: Culori.peAccent,
      surface: Culori.card,
      error: Culori.rosu,
    ),
    textTheme: text,
    textSelectionTheme: TextSelectionThemeData(
      cursorColor: Culori.accent,
      selectionColor: Culori.accentD,
      selectionHandleColor: Culori.accent,
    ),
    bottomSheetTheme: BottomSheetThemeData(
      backgroundColor: Culori.card,
      surfaceTintColor: Colors.transparent,
      showDragHandle: true,
      dragHandleColor: Culori.text2.withValues(alpha: 0.4),
      shape: const RoundedRectangleBorder(
        borderRadius: BorderRadius.vertical(top: Radius.circular(coltCard)),
      ),
    ),
    snackBarTheme: SnackBarThemeData(
      behavior: SnackBarBehavior.floating,
      backgroundColor: Culori.mesaj,
      contentTextStyle: fontText(marime: 14, culoare: Colors.white),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(99)),
    ),
  );
}
