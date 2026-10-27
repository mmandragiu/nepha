// ecranele cu casa din demo, pe un telefon ingust si pe tableta, ziua si noaptea:
// nimic nu trebuie sa iasa din chenar, iar cardul tinut apasat trebuie sa arate "Date avansate"
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:nepha/providers/app_state.dart';
import 'package:nepha/screens/acasa_screen.dart';
import 'package:nepha/screens/camere_screen.dart';
import 'package:nepha/screens/energie_screen.dart';
import 'package:nepha/services/api_service.dart';
import 'package:nepha/theme/nepha_theme.dart';
import 'package:provider/provider.dart';

AppState _casaDemo() {
  final demo = DemoCasa();
  return AppState()
    ..stare = demo.generateMockStare()
    ..istoric = demo.istoric()
    ..jurnal = demo.jurnal()
    ..rutina = demo.rutina()
    ..latentaMs = 42
    ..stareLa = DateTime.now();
}

// inaltime mare: lista construieste tot ecranul, nu doar ce incape (asa verificam si ce e jos)
Future<void> _arata(
  WidgetTester tester,
  Widget ecran,
  double latime, [
  double inaltime = 5000,
]) async {
  tester.view.physicalSize = Size(latime, inaltime);
  tester.view.devicePixelRatio = 1;
  addTearDown(tester.view.reset);
  await tester.pumpWidget(
    ChangeNotifierProvider.value(
      value: _casaDemo(),
      child: MaterialApp(
        theme: temaNepha(),
        home: Scaffold(body: ecran),
      ),
    ),
  );
  await tester.pump(const Duration(milliseconds: 600));
}

void main() {
  for (final noapte in [false, true]) {
    for (final latime in [340.0, 1024.0]) {
      final cum = '${noapte ? 'noaptea' : 'ziua'}, latime $latime';
      testWidgets('Acasa fara depasiri ($cum)', (tester) async {
        Culori.intunecat = noapte;
        await _arata(tester, AcasaScreen(mergiLa: (_) {}), latime);
        expect(tester.takeException(), isNull);
        expect(find.text('Casa acum'), findsOneWidget);
        expect(find.text('Jurnalul casei'), findsOneWidget);
      });
      testWidgets('Camere si Energie fara depasiri ($cum)', (tester) async {
        Culori.intunecat = noapte;
        await _arata(tester, const CamereScreen(), latime);
        expect(tester.takeException(), isNull);
        await _arata(tester, const EnergieScreen(), latime);
        expect(tester.takeException(), isNull);
      });
    }
  }

  testWidgets('cardul tinut apasat arata Date avansate', (tester) async {
    Culori.intunecat = true;
    await _arata(tester, AcasaScreen(mergiLa: (_) {}), 390, 900);
    final card = find.text('Baterie');
    await tester.scrollUntilVisible(card, 200);
    await tester.longPress(card);
    await tester.pump();
    await tester.pump(const Duration(milliseconds: 600));
    expect(tester.takeException(), isNull);
    expect(find.text('Date avansate'), findsOneWidget);
    expect(find.text('INA219 · I2C 0x44'), findsOneWidget);
    expect(find.text('42 ms'), findsOneWidget);
  });
}
