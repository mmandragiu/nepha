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
            systemNavigationBarColor: Culori.card,
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
      body: !gata
          ? Center(child: CircularProgressIndicator(color: Culori.verde))
          : IndexedStack(
              index: _fila,
              children: [
                AcasaScreen(mergiLa: _mergiLa),
                const CamereScreen(),
                const EnergieScreen(),
                const SecuritateScreen(),
              ],
            ),
      bottomNavigationBar: NavigationBar(
        selectedIndex: _fila,
        onDestinationSelected: _mergiLa,
        destinations: const [
          NavigationDestination(
            icon: Icon(Icons.home_outlined),
            selectedIcon: Icon(Icons.home),
            label: 'Acasă',
          ),
          NavigationDestination(
            icon: Icon(Icons.lightbulb_outline),
            selectedIcon: Icon(Icons.lightbulb),
            label: 'Camere',
          ),
          NavigationDestination(
            icon: Icon(Icons.bolt_outlined),
            selectedIcon: Icon(Icons.bolt),
            label: 'Energie',
          ),
          NavigationDestination(
            icon: Icon(Icons.shield_outlined),
            selectedIcon: Icon(Icons.shield),
            label: 'Securitate',
          ),
        ],
      ),
    );
  }
}
