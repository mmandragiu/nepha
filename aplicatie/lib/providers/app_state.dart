// app_state - tot ce stie aplicatia despre casa, intr-un singur loc
//
// - intreaba panoul o data la 2 secunde (ca pagina casei); jurnalul, graficul si rutina mai rar
// - tine minte adresa panoului, modul demo si temperatura de confort
// - comenzile se vad pe loc ("optimist"): comutatorul se muta imediat, iar daca panoul
//   nu raspunde in 3 secunde revine si apare un mesaj
// - amprenta / Face ID: PIN-ul il verifica tot Arduino. Dupa o dezarmare reusita cu PIN,
//   il pastram criptat pe telefon; data viitoare il trimitem noi, dupa amprenta.
import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter_secure_storage/flutter_secure_storage.dart';
import 'package:local_auth/local_auth.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../models/api_models.dart';
import '../services/api_service.dart';
import '../theme/nepha_theme.dart';

enum Conexiune {
  demo,
  faraAdresa,
  seConecteaza,
  conectat,
  arduinoTace,
  deconectat,
}

extension TextConexiune on Conexiune {
  String get text => switch (this) {
    Conexiune.demo => 'demo',
    Conexiune.faraAdresa => 'fără panou',
    Conexiune.seConecteaza => 'se conectează...',
    Conexiune.conectat => 'conectat la casă',
    Conexiune.arduinoTace => 'Arduino nu răspunde',
    Conexiune.deconectat => 'panoul nu răspunde',
  };

  // verde = merge, galben = asteptam / Arduino tace, rosu = panoul nu raspunde
  Color get culoare => switch (this) {
    Conexiune.conectat => Culori.verde,
    Conexiune.deconectat => Culori.rosu,
    Conexiune.faraAdresa => Culori.text2,
    _ => Culori.soare,
  };
}

class AppState extends ChangeNotifier {
  final _api = ApiService();
  final _demo = DemoCasa();
  final _seif = const FlutterSecureStorage();
  final _amprenta = LocalAuthentication();
  final mesageri =
      GlobalKey<ScaffoldMessengerState>(); // pentru mesajele de jos

  static const _pas = Duration(seconds: 2);
  static const _optimistCat = Duration(
    milliseconds: 4500,
  ); // cat aratam valoarea noua inainte s-o confirme panoul

  // setari
  String adresa = '';
  bool demo = true;
  double confort = 21.5;
  bool amprentaPornita = false; // utilizatorul vrea sa dezarmeze cu amprenta
  bool amprentaDisponibila = false; // telefonul are amprenta / Face ID
  bool areSalvatPin = false;
  bool setariGata = false;

  // ce stim despre casa
  Stare? stare;
  List<Decizie> jurnal = const [];
  Istoric? istoric;
  Rutina? rutina;
  Conexiune _conexiune = Conexiune.seConecteaza;

  final _optimist = <String, (Object, DateTime)>{};
  final _asteptari = <String>{};
  Timer? _ceas;
  int _runda = 0;
  int _generatie =
      0; // se schimba cand schimbam panoul: raspunsurile vechi nu mai conteaza
  String? _pinDeVerificat;
  DateTime? _pinTrimisLa;

  Conexiune get conexiune {
    if (demo) return Conexiune.demo;
    if (adresa.isEmpty) return Conexiune.faraAdresa;
    return _conexiune;
  }

  // ---------------------------------------------------------------- pornire

  Future<void> porneste() async {
    final p = await SharedPreferences.getInstance();
    adresa = p.getString('adresa') ?? '';
    demo = p.getBool('demo') ?? true;
    confort = p.getDouble('confort') ?? 21.5;
    amprentaPornita = p.getBool('amprenta') ?? false;
    if (!kIsWeb) {
      try {
        amprentaDisponibila =
            await _amprenta.isDeviceSupported() &&
            await _amprenta.canCheckBiometrics;
        areSalvatPin = await _seif.containsKey(key: 'pin');
      } catch (_) {
        amprentaDisponibila = false;
      }
    }
    setariGata = true;
    notifyListeners();
    _reporneste();
  }

  Future<void> schimbaSetari({
    String? adresa,
    bool? demo,
    double? confort,
    bool? amprenta,
  }) async {
    final p = await SharedPreferences.getInstance();
    final altPanou =
        (adresa != null && adresa != this.adresa) ||
        (demo != null && demo != this.demo);
    if (adresa != null) {
      this.adresa = adresa;
      await p.setString('adresa', adresa);
    }
    if (demo != null) {
      this.demo = demo;
      await p.setBool('demo', demo);
    }
    if (confort != null) {
      this.confort = confort;
      await p.setDouble('confort', confort);
    }
    if (amprenta != null) {
      amprentaPornita = amprenta;
      await p.setBool('amprenta', amprenta);
      if (!amprenta) await uitaPin();
    }
    if (altPanou) {
      stare = null; // ce stiam nu mai e valabil
      jurnal = const [];
      istoric = null;
      rutina = null;
      _conexiune = Conexiune.seConecteaza;
      _reporneste();
    }
    notifyListeners();
  }

  // ---------------------------------------------------------------- intrebam panoul

  void _reporneste() {
    _ceas?.cancel();
    _runda = 0;
    _generatie++;
    _intreaba();
  }

  void improspateaza() {
    _ceas?.cancel();
    _intreaba();
  }

  Future<void> _intreaba() async {
    final g = _generatie;
    final n = _runda++;
    try {
      if (demo) {
        stare = _demo.generateMockStare();
        if (n % 3 == 0) jurnal = _demo.jurnal();
        if (n % 15 == 0) istoric = _demo.istoric();
        if (n % 30 == 0) rutina = _demo.rutina();
      } else if (adresa.isNotEmpty) {
        final s = await _api.stare(adresa);
        if (g != _generatie) return;
        stare = s;
        _conexiune = s.arduinoVorbeste
            ? Conexiune.conectat
            : Conexiune.arduinoTace;
        final a = adresa;
        if (n % 3 == 0) {
          _api
              .jurnal(a)
              .then((j) => g == _generatie ? _pune(() => jurnal = j) : null)
              .ignore();
        }
        if (n % 15 == 0) {
          _api
              .istoric(a)
              .then((i) => g == _generatie ? _pune(() => istoric = i) : null)
              .ignore();
        }
        if (n % 30 == 0) {
          _api
              .rutina(a)
              .then((r) => g == _generatie ? _pune(() => rutina = r) : null)
              .ignore();
        }
      }
      _verificaPin();
    } catch (_) {
      if (g != _generatie) return;
      _conexiune = Conexiune.deconectat;
      _runda = 0; // cand revine, luam din nou tot
    }
    if (g != _generatie) return;
    notifyListeners();
    _ceas = Timer(_pas, _intreaba);
  }

  void _pune(void Function() f) {
    f();
    notifyListeners();
  }

  // ---------------------------------------------------------------- comenzi

  // trimite o comanda; cu `cheie`, interfata arata `valoare` pana confirma panoul
  Future<bool> comanda(String c, {String? cheie, Object? valoare}) async {
    if (cheie != null && valoare != null) {
      _optimist[cheie] = (
        valoare,
        DateTime.now().add(const Duration(minutes: 1)),
      );
      _asteptari.add(cheie);
      notifyListeners();
    }
    var ok = true;
    try {
      if (demo) {
        await Future<void>.delayed(
          const Duration(milliseconds: 250),
        ); // ca pe fir, nu chiar instant
        _demo.comanda(c);
      } else {
        if (adresa.isEmpty) throw StateError('fara adresa');
        await _api.comanda(adresa, c);
      }
    } catch (_) {
      ok = false;
      anunta('Panoul nu a putut fi contactat', eroare: true);
    }
    if (cheie != null) {
      _asteptari.remove(cheie);
      if (!ok) {
        _optimist.remove(cheie);
      } else if (valoare != null) {
        _optimist[cheie] = (valoare, DateTime.now().add(_optimistCat));
        Timer(_optimistCat + const Duration(milliseconds: 50), notifyListeners);
      }
    }
    notifyListeners();
    if (ok) {
      Timer(
        const Duration(milliseconds: 400),
        improspateaza,
      ); // aducem repede starea noua
    }
    return ok;
  }

  // valoarea de aratat: cea trimisa chiar acum, sau ce spune panoul
  T arata<T>(String cheie, T real) {
    final o = _optimist[cheie];
    if (o != null && o.$2.isAfter(DateTime.now()) && o.$1 is T) {
      return o.$1 as T;
    }
    return real;
  }

  bool asteapta(String cheie) => _asteptari.contains(cheie);

  void anunta(String text, {bool eroare = false}) {
    mesageri.currentState
      ?..hideCurrentSnackBar()
      ..showSnackBar(
        SnackBar(
          content: Text(text),
          backgroundColor: eroare ? Culori.rosu : null,
          duration: const Duration(milliseconds: 2800),
        ),
      );
  }

  // ---------------------------------------------------------------- alarma: PIN si amprenta

  Future<void> dezarmeazaCuPin(String pin) async {
    if (await comanda('ALARMA OPRIT $pin') && amprentaPornita) {
      // il pastram doar daca Arduino il accepta (alarma se dezarmeaza in cateva secunde)
      _pinDeVerificat = pin;
      _pinTrimisLa = DateTime.now();
    }
  }

  void _verificaPin() {
    final pin = _pinDeVerificat;
    if (pin == null || _pinTrimisLa == null) return;
    if (stare?.alarma.cod == 0) {
      _pinDeVerificat = null;
      _seif.write(key: 'pin', value: pin).then((_) {
        areSalvatPin = true;
        notifyListeners();
      }).ignore();
    } else if (DateTime.now().difference(_pinTrimisLa!) >
        const Duration(seconds: 8)) {
      _pinDeVerificat = null; // PIN gresit: nu-l tinem minte
    }
  }

  // true daca am trimis PIN-ul salvat
  Future<bool> dezarmeazaCuAmprenta() async {
    if (!amprentaPornita || !amprentaDisponibila) return false;
    final pin = await _seif.read(key: 'pin');
    if (pin == null) return false;
    try {
      final ok = await _amprenta.authenticate(
        localizedReason: 'Dezarmați alarma casei',
        biometricOnly: true,
      );
      if (!ok) return false;
    } on LocalAuthException catch (_) {
      anunta('Amprenta nu a mers. Folosiți PIN-ul.', eroare: true);
      return false;
    }
    await comanda('ALARMA OPRIT $pin');
    return true;
  }

  Future<void> uitaPin() async {
    await _seif.delete(key: 'pin');
    areSalvatPin = false;
    notifyListeners();
  }

  @override
  void dispose() {
    _ceas?.cancel();
    super.dispose();
  }
}
