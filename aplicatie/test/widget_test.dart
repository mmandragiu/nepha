// teste mici: numerele scrise ca la noi si JSON-ul panoului citit corect
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:nepha/models/api_models.dart';
import 'package:nepha/widgets/grafice.dart';
import 'package:nepha/widgets/ui.dart';

void main() {
  _harta();

  test('numere cu virgula si puncte la mii', () {
    expect(nr(21.44), '21,4');
    expect(nr(16200, 0), '16.200');
    expect(nr(null), '–');
    expect(grade(7.5), '7,5°');
  });

  test(
    'stare citita din JSON-ul panoului, cu null unde lipseste termometrul',
    () {
      final s = Stare.fromJson({
        'primit': true,
        'vechime': 2,
        'ora': '18:52',
        'minut': 1132,
        'mod': 'smart',
        'test': 0,
        'camere': [
          {
            'nume': 'Sufragerie',
            'temp': 21.4,
            'lumina': 80,
            'prezenta': true,
            'fereastra': false,
          },
          {
            'nume': 'Dormitor',
            'temp': null,
            'lumina': 0,
            'prezenta': false,
            'fereastra': true,
          },
        ],
        'afara': null,
        'energie': {
          'baterie': 78,
          'solar': 0.12,
          'whAzi': 5230,
          'whClasic': 7480,
          'mod': 'Normal',
          'modCod': 0,
        },
        'alarma': {
          'stare': 'Dezarmată',
          'cod': 0,
          'senzori': [
            {'nume': 'Ușa de la intrare', 'deschis': false},
          ],
        },
        'vreme': {'valida': false},
        'rezultate': [
          {'valid': false},
          {'valid': true, 'total': 13681},
        ],
      });
      expect(s.smart, isTrue);
      expect(s.arduinoVorbeste, isTrue);
      expect(s.camere[1].temp, isNull);
      expect(s.tempCasa, 21.4);
      expect(s.afara, isNull);
      expect(s.energie.kwhEconomisiti, closeTo(2.25, 0.001));
      expect(s.alarma.senzori.single.nume, 'Ușa de la intrare');
      expect(s.vreme.oraPloaie, isNull);
      expect(s.rezultate[1].total, 13681);
      expect(s.areMiscare, isFalse);
    },
  );

  test(
    'fara senzori de miscare rutina e un singur rand, cu maximul pe ore',
    () {
      final r = Rutina(
        valida: true,
        camere: const ['Sufragerie', 'Dormitor', 'Bucătărie'],
        p: [
          List.filled(24, 20),
          List.filled(24, 70),
          List.generate(24, (h) => h == 8 ? 90 : 10),
        ],
        zile: List.filled(24, 3),
      ).caAcasa();
      expect(r.camere, ['Acasă']);
      expect(r.p.single[0], 70);
      expect(r.p.single[8], 90);
      expect(r.zile.first, 3);
    },
  );
}

// harta rutinei intr-o lista care se deruleaza (ca pe ecranul Camere), pe telefon si pe tableta:
// lista trebuie sa se termine dupa harta, nu sa mearga la nesfarsit in gol
void _harta() {
  for (final latime in [390.0, 1280.0]) {
    testWidgets('harta rutinei are inaltime normala (latime $latime)', (
      tester,
    ) async {
      tester.view.physicalSize = Size(latime, 800);
      tester.view.devicePixelRatio = 1;
      addTearDown(tester.view.reset);
      final r = Rutina(
        valida: true,
        camere: const ['Sufragerie', 'Dormitor', 'Bucătărie'],
        p: List.generate(3, (_) => List.generate(24, (h) => h * 4)),
        zile: List.filled(24, 7),
      );
      await tester.pumpWidget(
        MaterialApp(
          home: Scaffold(
            body: ListView(children: [HartaRutina(r, oraAcum: 18)]),
          ),
        ),
      );
      expect(tester.takeException(), isNull);
      final lista = tester.state<ScrollableState>(find.byType(Scrollable));
      expect(lista.position.maxScrollExtent, 0);
      expect(tester.getSize(find.byType(HartaRutina)).height, lessThan(200));
    });
  }
}
