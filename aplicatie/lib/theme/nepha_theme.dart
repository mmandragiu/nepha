// tema NEPHA - aceleasi culori, colturi si font ca pagina casei si ecranul panoului
import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

class Culori {
  static const fundal = Color(0xFFF7F4EF); // alb cald
  static const card = Color(0xFFFFFFFF);
  static const linie = Color(0xFFE8E3DA);
  static const moale = Color(0xFFEFEAE2); // fundalul bucatilor din carduri
  static const text = Color(0xFF2B2A28);
  static const text2 = Color(0xFF8A857C);
  static const verde = Color(0xFF3F8F5E);
  static const verdeD = Color(0xFFE3F0E7);
  static const soare = Color(0xFFDE9428);
  static const soareD = Color(0xFFFBEBD0);
  static const apa = Color(0xFF3D86BD);
  static const apaD = Color(0xFFDDEBF6);
  static const cald = Color(0xFFD7684B);
  static const caldD = Color(0xFFF9E1DA);
  static const rosu = Color(0xFFCF3F3F);
  static const rosuD = Color(0xFFF8DADA);
}

// aceeasi curba ca pe pagina si pe panou: porneste hotarat, se opreste bland
const usor = Cubic(0.4, 0, 0.2, 1);

const coltCard = 14.0;

// umbra fina de sub carduri
const umbraCard = [
  BoxShadow(color: Color(0x0F2B2A28), blurRadius: 2, offset: Offset(0, 1)),
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
    brightness: Brightness.light,
    scaffoldBackgroundColor: Culori.fundal,
    colorScheme: ColorScheme.fromSeed(
      seedColor: Culori.verde,
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
        side: const BorderSide(color: Culori.linie, width: 0.6),
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
      backgroundColor: const Color(0xFF26231F),
      contentTextStyle: GoogleFonts.nunito(color: Colors.white, fontSize: 14),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(99)),
    ),
  );
}
