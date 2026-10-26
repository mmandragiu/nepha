// api_service - vorbim cu panoul pe HTTP, la fel ca pagina casei (docs/protocol.md)
//   GET  /api/stare, /api/jurnal, /api/istoric, /api/rutina
//   POST /api/comanda   c=<comanda>   (ex. "LUMINA 0 100", "ALARMA PLECAT")
//   POST /api/imperechere cod=<codul de pe ecranul panoului>&client=<id telefon>
//        -> numele panoului, cheia casei si contul telefonului pe broker
// In modul demo nu vorbim cu nimeni: o seara simulata, in care comenzile chiar schimba starea.
import 'dart:convert';
import 'dart:math';

import 'package:dio/dio.dart';

import '../models/api_models.dart';

const _astept = Duration(
  seconds: 3,
); // daca panoul nu raspunde in 3 s, renuntam

// "192.168.1.42" sau "nepha.local/" -> "http://192.168.1.42"
String adresaCompleta(String adresa) {
  var a = adresa.trim().replaceAll(RegExp(r'/+$'), '');
  if (a.isNotEmpty &&
      !RegExp(r'^https?://', caseSensitive: false).hasMatch(a)) {
    a = 'http://$a';
  }
  return a;
}

class ApiService {
  final _dio = Dio(
    BaseOptions(
      connectTimeout: _astept,
      receiveTimeout: _astept,
      sendTimeout: _astept,
      headers: {'Cache-Control': 'no-store'},
    ),
  );

  Future<Map<String, dynamic>> _obiect(String adresa, String cale) async {
    final r = await _dio.get<Map<String, dynamic>>(
      '${adresaCompleta(adresa)}$cale',
    );
    return r.data ?? const {};
  }

  Future<Stare> stare(String adresa) async =>
      Stare.fromJson(await _obiect(adresa, '/api/stare'));

  Future<Istoric> istoric(String adresa) async =>
      Istoric.fromJson(await _obiect(adresa, '/api/istoric'));

  Future<Rutina> rutina(String adresa) async =>
      Rutina.fromJson(await _obiect(adresa, '/api/rutina'));

  Future<List<Decizie>> jurnal(String adresa) async {
    final r = await _dio.get<List<dynamic>>(
      '${adresaCompleta(adresa)}/api/jurnal',
    );
    return (r.data ?? const [])
        .map((e) => Decizie.fromJson(e as Map<String, dynamic>))
        .toList();
  }

  // codul de 6 cifre de pe panou (Setari -> Leaga telefonul) -> numele panoului, cheia casei si
  // contul pe broker (utilizator / parola goale daca panoul n-a putut face contul: doar Wi-Fi);
  // null daca panoul nu accepta codul (gresit sau expirat)
  Future<({String server, List<int> cheie, String utilizator, String parola})?>
  imperecheaza(String adresa, String cod, String client) async {
    try {
      final r = await _dio.post<Map<String, dynamic>>(
        '${adresaCompleta(adresa)}/api/imperechere',
        data: {'cod': cod, 'client': client},
        options: Options(
          contentType: Headers.formUrlEncodedContentType,
          receiveTimeout: const Duration(
            seconds: 20,
          ), // panoul face si contul pe broker
        ),
      );
      final j = r.data ?? const {};
      if (j['ok'] != true) return null;
      return (
        server: '${j['server']}',
        cheie: base64.decode('${j['cheie']}'),
        utilizator: '${j['utilizator'] ?? ''}',
        parola: '${j['parola'] ?? ''}',
      );
    } on DioException catch (e) {
      if (e.response?.statusCode == 403) return null;
      rethrow;
    }
  }

  // arunca o eroare daca panoul nu raspunde sau nu accepta comanda
  Future<void> comanda(String adresa, String c) async {
    await _dio.post<dynamic>(
      '${adresaCompleta(adresa)}/api/comanda',
      data: {'c': c},
      options: Options(contentType: Headers.formUrlEncodedContentType),
    );
  }
}

// ---------------------------------------------------------------- modul demo

class DemoCasa {
  final _inceput = DateTime.now();
  String _mod = 'smart';
  int _alarma = 0;
  DateTime? _armatLa;
  DateTime? _pompaPanaLa;
  final _lumini = [80, 0, 45];
  final _usi = [false, false, false, false];

  static const _numeAlarma = [
    'Dezarmată',
    'Se armează...',
    'Armată (noapte)',
    'Armată (plecat)',
    'Introduceți PIN-ul!',
    'ALARMĂ!',
  ];
  static const _rutina = [
    [
      10,
      11,
      11,
      11,
      11,
      11,
      11,
      88,
      66,
      11,
      11,
      11,
      11,
      11,
      11,
      11,
      11,
      77,
      88,
      88,
      88,
      88,
      88,
      11,
    ],
    [
      90,
      88,
      88,
      88,
      88,
      88,
      88,
      11,
      11,
      11,
      11,
      11,
      11,
      11,
      11,
      11,
      11,
      11,
      11,
      11,
      11,
      11,
      77,
      88,
    ],
    [
      10,
      11,
      11,
      11,
      11,
      11,
      88,
      88,
      22,
      11,
      11,
      11,
      11,
      11,
      11,
      11,
      11,
      11,
      77,
      88,
      11,
      11,
      11,
      11,
    ],
  ];

  double get _secunde =>
      DateTime.now().difference(_inceput).inMilliseconds / 1000;
  int get _minut =>
      (18 * 60 + 40 + _secunde.floor()) % 1440; // un minut pe secunda

  static String _ora(int m) =>
      '${(m ~/ 60 % 24).toString().padLeft(2, '0')}:${(m % 60).toString().padLeft(2, '0')}';

  void comanda(String c) {
    if (c == 'MOD SMART') _mod = 'smart';
    if (c == 'MOD CLASIC') _mod = 'clasic';
    if (c == 'ALARMA PLECAT') {
      _alarma = 1; // ca pe casa adevarata: intai avem timp sa iesim
      _armatLa = DateTime.now().add(const Duration(seconds: 5));
    }
    if (c == 'ALARMA ACASA') _alarma = 2;
    if (c.startsWith('ALARMA OPRIT') && c.endsWith(' 1234')) {
      _alarma = 0; // PIN-ul din demo
    }
    if (c == 'UDA') {
      _pompaPanaLa = DateTime.now().add(const Duration(seconds: 8));
    }
    final l = RegExp(r'^LUMINA (\d) (\d+)$').firstMatch(c);
    if (l != null) _lumini[int.parse(l[1]!)] = int.parse(l[2]!);
    final u = RegExp(r'^USA (\d) (\d)$').firstMatch(c);
    if (u != null) _usi[int.parse(u[1]!)] = u[2] == '1';
  }

  // seara simulata; energia, bateria si temperaturile se misca usor de la un apel la altul
  Stare generateMockStare() {
    final u = _secunde;
    if (_alarma == 1 && DateTime.now().isAfter(_armatLa!)) _alarma = 3;
    final pompa =
        _pompaPanaLa != null && DateTime.now().isBefore(_pompaPanaLa!);
    return Stare(
      primit: true,
      vechime: 0,
      ora: _ora(_minut),
      minut: _minut,
      mod: _mod,
      test: 0,
      camere: [
        Camera(
          nume: 'Sufragerie',
          temp: 21.4 + sin(u / 20) * 0.2,
          lumina: _lumini[0],
          prezenta: true,
          fereastra: false,
        ),
        Camera(
          nume: 'Dormitor',
          temp: 19.6 + sin(u / 31) * 0.1,
          lumina: _lumini[1],
          prezenta: false,
          fereastra: _usi[2],
        ),
        Camera(
          nume: 'Bucătărie',
          temp: 21.0 + cos(u / 25) * 0.15,
          lumina: _lumini[2],
          prezenta: true,
          fereastra: false,
        ),
      ],
      afara: 7.5 + sin(u / 60) * 0.3,
      luminaNaturala: 8,
      incalzire: Incalzire(
        pornita: sin(u / 15) > 0,
        tinta: 21.5,
        motiv: 'e cineva acasă',
        motivCod: 1,
        rata: 3.4,
      ),
      energie: Energie(
        baterie: (78 + sin(u / 40) * 2).round(),
        solar: max(0, 0.08 + sin(u / 9) * 0.03),
        vant: 0.31 + sin(u / 7) * 0.05,
        consum: 1.2 + cos(u / 8) * 0.1 + (pompa ? 0.35 : 0),
        mod: 'Normal',
        modCod: 0,
        whAzi: 5230 + u * 2,
        whClasic: 7480 + u * 3,
        mwh: 0,
        estimatSolar6h: 0.4,
        factorPanou: 0.82,
      ),
      alarma: Alarma(
        stare: _numeAlarma[_alarma],
        cod: _alarma,
        senzori: [
          SenzorUsa(nume: 'Ușa de la intrare', deschis: _usi[0]),
          SenzorUsa(nume: 'Ușa spre curte', deschis: _usi[1]),
          SenzorUsa(nume: 'Geam dormitor', deschis: _usi[2]),
          SenzorUsa(nume: 'Geam sufragerie', deschis: _usi[3]),
        ],
      ),
      gradina: Gradina(umiditate: pompa ? 34 : 31, pompa: pompa),
      priza: true,
      senzori: 159,
      vreme: const Vreme(
        valida: true,
        temp: 7.8,
        ploaie: 80,
        oraPloaie: 21,
        soare: 0,
        nori: 90,
        vant: 14,
      ),
      rezultate: const [
        RezultatTest(
          valid: true,
          lumini: 177,
          incalzire: 16200,
          pompa: 60,
          priza: 397,
          total: 16834,
          litri: 80,
        ),
        RezultatTest(
          valid: true,
          lumini: 91,
          incalzire: 13333,
          pompa: 0,
          priza: 257,
          total: 13681,
          litri: 0,
        ),
      ],
      retea: const Retea(ip: '', nume: '', ap: false),
      pretKwh: 1.4,
    );
  }

  List<Decizie> jurnal() => const [
    Decizie(
      ora: '18:32',
      text: 'Am stins lumina în Bucătărie: nu mai era nimeni.',
    ),
    Decizie(ora: '17:32', text: 'Alarma a fost dezarmată. Bine ați venit!'),
    Decizie(
      ora: '15:08',
      text:
          'Pornesc încălzirea mai devreme: de obicei e cineva acasă pe la 17:00 (peste ~112 min).',
    ),
    Decizie(
      ora: '08:12',
      text: 'Am oprit priza televizorului: stătea în standby de 16 min.',
    ),
    Decizie(
      ora: '08:00',
      text: 'Alarma e armată. Casa știe că nu e nimeni și economisește.',
    ),
    Decizie(
      ora: '06:48',
      text:
          'În Bucătărie temperatura scade brusc, pare o fereastră deschisă. Opresc încălzirea 15 minute.',
    ),
    Decizie(
      ora: '04:52',
      text:
          'Pornesc încălzirea mai devreme: de obicei e cineva acasă pe la 6:00 (peste ~68 min).',
    ),
    Decizie(
      ora: '00:04',
      text: 'Nu ud grădina: vine ploaia (80% pe la ora 16).',
    ),
  ];

  Istoric istoric() {
    final minut = <int>[],
        solar = <double>[],
        vant = <double>[],
        consum = <double>[];
    for (var m = 0; m < 1130; m += 5) {
      final zi = max(0.0, sin((m / 60 - 7) / 11 * pi));
      minut.add(m);
      solar.add(1.9 * zi * (m > 960 && m < 1080 ? 0.35 : 1));
      vant.add(0.12 + 0.3 * max(0.0, sin(m / 190)));
      final acasa = m < 480 || m > 1050;
      final calorifer = (acasa || m > 900) && sin(m / 23) > 0.1
          ? 1
          : 0; // porneste si se opreste
      consum.add((acasa ? 0.45 : 0.15) + calorifer);
    }
    return Istoric(
      minut: minut,
      solar: solar,
      vant: vant,
      consum: consum,
      baterie: const [],
      tCasa: const [],
      tAfara: const [],
    );
  }

  Rutina rutina() => Rutina(
    valida: true,
    camere: const ['Sufragerie', 'Dormitor', 'Bucătărie'],
    p: _rutina,
    zile: List.filled(24, 7),
  );
}
