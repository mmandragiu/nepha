// teste mici: numerele scrise ca la noi si JSON-ul panoului citit corect
import 'package:flutter_test/flutter_test.dart';
import 'package:nepha/models/api_models.dart';
import 'package:nepha/widgets/ui.dart';

void main() {
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
    },
  );
}
