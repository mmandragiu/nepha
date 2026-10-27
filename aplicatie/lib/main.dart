// NEPHA - aplicatia de telefon. Nu ia nicio decizie: arata ce face casa si trimite comenzile
// noastre, prin panoul ESP32. Creierul ramane Arduino.
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';

import 'providers/app_state.dart';
import 'screens/acasa_screen.dart';
import 'screens/camere_screen.dart';
import 'screens/energie_screen.dart';
import 'screens/securitate_screen.dart';
import 'theme/nepha_theme.dart';
import 'widgets/ui.dart';

void main() {
  runApp(
    ChangeNotifierProvider(
      create: (_) => AppState()..porneste(),
      child: const Nepha(),
    ),
  );
}

// tema de zi sau de noapte: dupa telefon ("auto") sau cum am ales la Setari
class Nepha extends StatelessWidget {
  const Nepha({super.key});

  // culorile nu sunt constante: cand se schimba tema, refacem tot ecranul, ca sa le citeasca din nou
  void _refaTot(Element e) {
    e.markNeedsBuild();
    e.visitChildren(_refaTot);
  }

  @override
  Widget build(BuildContext context) {
    final tema = context.select<AppState, String>((s) => s.tema);
    final noapte = tema == 'auto'
        ? MediaQuery.platformBrightnessOf(context) == Brightness.dark
        : tema == 'noapte';
    if (noapte != Culori.intunecat) {
      Culori.intunecat = noapte;
      (context as Element).visitChildren(_refaTot);
    }
    return AnnotatedRegion<SystemUiOverlayStyle>(
      // ceasul si bateria de sus: negre ziua, albe noaptea
      value: (noapte ? SystemUiOverlayStyle.light : SystemUiOverlayStyle.dark)
          .copyWith(
            statusBarColor: Colors.transparent,
            systemNavigationBarColor: Colors.transparent,
          ),
      child: MaterialApp(
        title: 'NEPHA',
        debugShowCheckedModeBanner: false,
        theme: temaNepha(),
        scaffoldMessengerKey: context.read<AppState>().mesageri,
        home: const Acasa(),
      ),
    );
  }
}

class Acasa extends StatefulWidget {
  const Acasa({super.key});

  @override
  State<Acasa> createState() => _AcasaState();
}

class _AcasaState extends State<Acasa> {
  int _fila = 0;

  void _mergiLa(int f) => setState(() => _fila = f);

  @override
  Widget build(BuildContext context) {
    final gata = context.select<AppState, bool>((s) => s.setariGata);
    return Scaffold(
      backgroundColor: Culori.fundal,
      body: Stack(
        children: [
          if (!gata)
            Center(child: CircularProgressIndicator(color: Culori.accent))
          else
            IndexedStack(
              index: _fila,
              children: [
                AcasaScreen(mergiLa: _mergiLa),
                const CamereScreen(),
                const EnergieScreen(),
                const SecuritateScreen(),
              ],
            ),
          Positioned(
            left: 0,
            right: 0,
            bottom: 0,
            child: _BaraDeJos(fila: _fila, mergiLa: _mergiLa),
          ),
        ],
      ),
    );
  }
}

const _file = [
  (Icons.home_outlined, Icons.home_rounded, 'Acasă'),
  (Icons.lightbulb_outline, Icons.lightbulb, 'Camere'),
  (Icons.bolt_outlined, Icons.bolt, 'Energie'),
  (Icons.shield_outlined, Icons.shield, 'Securitate'),
];

// bara de jos, ca in poza: o capsula de sticla fumurie care pluteste peste ecran (ce trece pe sub
// ea se vede incetosat), cu iconitele in cercuri; fila deschisa e cercul portocaliu
class _BaraDeJos extends StatelessWidget {
  final int fila;
  final void Function(int) mergiLa;
  const _BaraDeJos({required this.fila, required this.mergiLa});

  Widget _buton(int i, {required bool aleasa}) => Semantics(
    button: true,
    selected: aleasa,
    label: _file[i].$3,
    child: GestureDetector(
      onTap: () {
        if (!aleasa) atingere();
        mergiLa(i);
      },
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 250),
        curve: usor,
        width: 52,
        height: 52,
        decoration: BoxDecoration(
          color: aleasa ? Culori.accent : Colors.white.withValues(alpha: 0.1),
          shape: BoxShape.circle,
        ),
        child: Icon(
          aleasa ? _file[i].$2 : _file[i].$1,
          size: 22,
          color: aleasa ? Colors.white : Colors.white.withValues(alpha: 0.75),
        ),
      ),
    ),
  );

  // iconitele stau mereu in aceeasi ordine (nu sar dintr-un loc in altul cand schimbi fila)
  @override
  Widget build(BuildContext context) => SafeArea(
    top: false,
    minimum: const EdgeInsets.only(bottom: 12),
    child: Center(
      child: Sticla(
        fumurie: true,
        padding: const EdgeInsets.all(5),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            for (var i = 0; i < _file.length; i++)
              Padding(
                padding: const EdgeInsets.symmetric(horizontal: 3),
                child: _buton(i, aleasa: i == fila),
              ),
          ],
        ),
      ),
    ),
  );
}
