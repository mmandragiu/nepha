// app_state - tot ce stie aplicatia despre casa, intr-un singur loc
//
// Doua drumuri spre casa:
// - acasa, pe Wi-Fi: intrebam panoul direct, o data la 2 secunde (jurnalul, graficul si
//   rutina mai rar). E drumul pe care il incercam mereu intai.
// - prin internet (telefonul e pe date mobile sau in alta parte): prin brokerul MQTT
//   (services/broker.dart). Panoul ne anunta singur ce se schimba; noi ii zicem doar
//   "ma uit" o data pe minut. La 20 de secunde mai incercam o data Wi-Fi-ul: cand am ajuns
//   acasa, trecem singuri inapoi.
// Cand aplicatia nu e pe ecran, nu intrebam nimic (nu consumam trafic degeaba).
//
// - tine minte adresa panoului, modul demo, temperatura de confort si tema (zi / noapte)
// - graficul pe 3 zile il tinem pe telefon (services/istoric_local.dart)
// - comenzile se vad pe loc ("optimist"): comutatorul se muta imediat, iar daca panoul
//   nu raspunde in cateva secunde revine si apare un mesaj
// - Face ID / Touch ID pe iPhone, amprenta pe Android: PIN-ul il verifica tot Arduino.
//   Dupa o dezarmare reusita cu PIN, il pastram criptat pe telefon (Keychain pe iPhone,
//   Keystore pe Android); data viitoare il trimitem noi, dupa ce telefonul ne recunoaste.
// - legarea de casa (imperecherea): pe Wi-Fi, cu codul de pe panou, primim cheia casei.
//   Cu ea criptam PIN-ul cand dezarmam prin internet (brokerul nu-l vede niciodata).
import 'dart:async';
import 'dart:convert';
import 'dart:math';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter_secure_storage/flutter_secure_storage.dart';
import 'package:local_auth/local_auth.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../models/api_models.dart';
import '../services/api_service.dart';
import '../services/broker.dart';
import '../services/istoric_local.dart';
import '../theme/nepha_theme.dart';

enum Conexiune {
  demo,
  faraAdresa,
  seConecteaza,
  conectat,
  internet,
  arduinoTace,
  casaOffline,
  deconectat,
}

extension TextConexiune on Conexiune {
  String get text => switch (this) {
    Conexiune.demo => 'demo',
    Conexiune.faraAdresa => 'fără panou',
    Conexiune.seConecteaza => 'se conectează...',
    Conexiune.conectat => 'acasă, pe Wi-Fi',
    Conexiune.internet => 'prin internet',
    Conexiune.arduinoTace => 'Arduino nu răspunde',
    Conexiune.casaOffline => 'casa nu e pe internet',
    Conexiune.deconectat => 'panoul nu răspunde',
  };

  // verde = acasa, albastru = prin internet, galben = asteptam / Arduino tace, rosu = nu ajungem la casa
  Color get culoare => switch (this) {
    Conexiune.conectat => Culori.verde,
    Conexiune.internet => Culori.apa,
    Conexiune.deconectat || Conexiune.casaOffline => Culori.rosu,
    Conexiune.faraAdresa => Culori.text2,
    _ => Culori.soare,
  };
}

class AppState extends ChangeNotifier with WidgetsBindingObserver {
  final _api = ApiService();
  final _demo = DemoCasa();
  final _seif = const FlutterSecureStorage();
  final _amprenta = LocalAuthentication();
  final _local = IstoricLocal();
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
  String tema = 'auto'; // 'auto' (ca telefonul), 'zi' sau 'noapte'
  bool amprentaPornita = false; // utilizatorul vrea sa dezarmeze cu amprenta
  bool amprentaDisponibila =
      false; // telefonul are amprenta / Face ID / Touch ID
  // cum se numeste pe telefonul asta: "Face ID", "Touch ID" (iPhone) sau "amprenta" (Android)
  String numeBiometrie = 'amprenta';
  bool areSalvatPin = false;
  bool setariGata = false;

  // cine suntem pe broker si cu ce casa vorbim
  String clientId = ''; // id-ul telefonului, facut o data la prima pornire
  String server = ''; // numele panoului (ex. nepha-a1b2c3), aflat pe Wi-Fi
  List<int>? _cheie; // cheia casei, de la imperechere
  bool get legat => _cheie != null && server.isNotEmpty;
  bool get poatePrinInternet => areBroker && server.isNotEmpty;

  // ce stim despre casa
  Stare? stare;
  List<Decizie> jurnal = const [];
  Istoric? istoric; // ultimele 24 de ore, de la panou
  Istoric? istoric3; // ultimele 3 zile, de pe telefon
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

  Broker? _broker;
  bool _peInternet = false;
  int _rundaInternet = 0;
  bool _panouPeBroker = true;
  bool _vreauStare = false;
  DateTime _ultimaIncercareAcasa = DateTime(2000);

  Conexiune get conexiune {
    if (demo) return Conexiune.demo;
    if (adresa.isEmpty && !poatePrinInternet) return Conexiune.faraAdresa;
    return _conexiune;
  }

  bool get peInternet => !demo && _peInternet;

  // ---------------------------------------------------------------- pornire

  Future<void> porneste() async {
    final p = await SharedPreferences.getInstance();
    adresa = p.getString('adresa') ?? '';
    demo = p.getBool('demo') ?? true;
    confort = p.getDouble('confort') ?? 21.5;
    tema = p.getString('tema') ?? 'auto';
    amprentaPornita = p.getBool('amprenta') ?? false;
    server = p.getString('server') ?? '';
    clientId = p.getString('client') ?? '';
    if (clientId.isEmpty) {
      final r = Random.secure();
      const litere = 'abcdefghijklmnopqrstuvwxyz0123456789';
      clientId =
          'nepha-app-${List.generate(10, (_) => litere[r.nextInt(litere.length)]).join()}';
      await p.setString('client', clientId);
    }
    if (!kIsWeb) {
      try {
        amprentaDisponibila =
            await _amprenta.isDeviceSupported() &&
            await _amprenta.canCheckBiometrics;
        if (amprentaDisponibila) {
          final feluri = await _amprenta.getAvailableBiometrics();
          final iPhone = defaultTargetPlatform == TargetPlatform.iOS;
          if (iPhone && feluri.contains(BiometricType.face)) {
            numeBiometrie = 'Face ID';
          } else if (iPhone) {
            numeBiometrie = 'Touch ID';
          } else if (feluri.length == 1 && feluri.first == BiometricType.face) {
            numeBiometrie = 'recunoașterea feței';
          }
        }
        areSalvatPin = await _seif.containsKey(key: 'pin');
      } catch (_) {
        amprentaDisponibila = false;
      }
      try {
        final c = await _seif.read(key: 'cheie');
        if (c != null) _cheie = base64.decode(c);
      } catch (_) {}
    }
    try {
      istoric3 = await _local
          .citeste(); // graficul se vede imediat, chiar fara legatura
    } catch (_) {}
    setariGata = true;
    WidgetsBinding.instance.addObserver(this);
    notifyListeners();
    _reporneste();
  }

  // aplicatia iese de pe ecran / revine: nu intrebam nimic cat nu se uita nimeni
  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.paused) {
      _ceas?.cancel();
      _generatie++;
      _opresteBroker();
    } else if (state == AppLifecycleState.resumed && setariGata) {
      _reporneste();
    }
  }

  Future<void> schimbaSetari({
    String? adresa,
    bool? demo,
    double? confort,
    String? tema,
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
    if (tema != null) {
      this.tema = tema;
      await p.setString('tema', tema);
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
    _opresteBroker();
    _ultimaIncercareAcasa = DateTime(2000);
    _intreaba();
  }

  void improspateaza() {
    _ceas?.cancel();
    _vreauStare = true;
    _ultimaIncercareAcasa = DateTime(
      2000,
    ); // tragem in jos: incercam si Wi-Fi-ul
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
      } else if (!await _acasa(g, n)) {
        if (g != _generatie) return;
        await _prinInternet(g);
      }
      _verificaPin();
    } catch (_) {
      if (g != _generatie) return;
      _conexiune = _peInternet && !_panouPeBroker
          ? Conexiune.casaOffline
          : Conexiune.deconectat;
      _runda = 0; // cand revine, luam din nou tot
    }
    if (g != _generatie) return;
    notifyListeners();
    _ceas = Timer(_pas, _intreaba);
  }

  // drumul 1: panoul direct, pe Wi-Fi. false = nu l-am gasit (sau inca nu-l reincercam)
  Future<bool> _acasa(int g, int n) async {
    if (adresa.isEmpty) return false;
    final acum = DateTime.now();
    if (_peInternet &&
        acum.difference(_ultimaIncercareAcasa) < const Duration(seconds: 20)) {
      return false;
    }
    _ultimaIncercareAcasa = acum;
    final Stare s;
    try {
      s = await _api.stare(adresa);
    } catch (_) {
      if (!poatePrinInternet) rethrow; // n-avem alt drum
      return false;
    }
    if (g != _generatie) return true;
    if (_peInternet) {
      _peInternet = false; // am ajuns acasa: inchidem drumul prin internet
      _opresteBroker();
      n = 0;
      _runda = 1;
    }
    await _aflaServerul(s.server);
    stare = s;
    _conexiune = s.arduinoVorbeste ? Conexiune.conectat : Conexiune.arduinoTace;
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
          .then((i) => g == _generatie ? _istoricNou(i) : null)
          .ignore();
    }
    if (n % 30 == 0) {
      _api
          .rutina(a)
          .then((r) => g == _generatie ? _pune(() => rutina = r) : null)
          .ignore();
    }
    return true;
  }

  // drumul 2: prin broker. Starea vine singura (_dinBroker); noi doar cerem din cand in cand
  Future<void> _prinInternet(int g) async {
    if (!poatePrinInternet) throw StateError('fara drum prin internet');
    final b = _broker ??= Broker(
      client: clientId,
      server: server,
      laStare: (s) {
        if (g != _generatie) return;
        _panouPeBroker = true;
        stare = s;
        _conexiune = s.arduinoVorbeste
            ? Conexiune.internet
            : Conexiune.arduinoTace;
        _verificaPin();
        notifyListeners();
      },
      laEveniment: (d) {
        if (g != _generatie) return;
        _pune(() => jurnal = [d, ...jurnal.take(39)]);
      },
      laPanou: (online) {
        if (g != _generatie) return;
        _panouPeBroker = online;
        if (online) {
          _vreauStare = true;
        } else {
          _pune(() => _conexiune = Conexiune.casaOffline);
        }
      },
    );
    if (!b.conectat && !await b.porneste()) {
      throw StateError('brokerul nu raspunde');
    }
    if (g != _generatie) return;
    final primaData = !_peInternet;
    _peInternet = true;
    final n = primaData ? 0 : ++_rundaInternet;
    if (primaData) _rundaInternet = 0;
    try {
      // la 2 s o runda: "ma uit" la un minut, graficul la 5, rutina la 10
      if (n % 30 == 0) await b.privesc();
      if (primaData || _vreauStare) {
        _vreauStare = false;
        final s = await b.stare();
        if (g != _generatie) return;
        stare = s;
      }
      _panouPeBroker = true;
      if (stare != null) {
        _conexiune = stare!.arduinoVorbeste
            ? Conexiune.internet
            : Conexiune.arduinoTace;
      }
      // la intrare: jurnalul, ca sa vedem ce s-a intamplat cat aplicatia a fost inchisa
      if (primaData) {
        b
            .jurnal()
            .then((j) => g == _generatie ? _pune(() => jurnal = j) : null)
            .ignore();
      }
      if (n % 150 == 0) {
        b
            .istoric()
            .then((i) => g == _generatie ? _istoricNou(i) : null)
            .ignore();
      }
      if (n % 300 == 0) {
        b
            .rutina()
            .then((r) => g == _generatie ? _pune(() => rutina = r) : null)
            .ignore();
      }
    } on TimeoutException {
      _panouPeBroker = false; // brokerul merge, dar panoul nu raspunde
      rethrow;
    }
  }

  void _opresteBroker() {
    _broker?.opreste();
    _broker = null;
    _peInternet = false;
    _panouPeBroker = true;
  }

  // numele panoului il aflam pe Wi-Fi; daca e alt panou, cheia veche nu mai e buna
  Future<void> _aflaServerul(String s) async {
    if (s.isEmpty || s == server) return;
    final p = await SharedPreferences.getInstance();
    if (server.isNotEmpty && _cheie != null) {
      _cheie = null;
      await _seif.delete(key: 'cheie');
    }
    server = s;
    await p.setString('server', s);
  }

  Future<void> _istoricNou(Istoric i) async {
    istoric = i;
    notifyListeners();
    final s = stare;
    // in ziua de test ceasul casei merge mai repede: punctele nu sunt ore adevarate
    if (s == null || s.test != 0) return;
    try {
      await _local.adauga(i, s.minut);
      istoric3 = await _local.citeste();
      notifyListeners();
    } catch (_) {}
  }

  void _pune(void Function() f) {
    f();
    notifyListeners();
  }

  // ---------------------------------------------------------------- legarea de casa

  // codul de 6 cifre de pe panou (Setari -> Leaga telefonul); merge doar pe Wi-Fi-ul casei
  Future<bool> leagaTelefonul(String cod) async {
    if (adresa.isEmpty || _peInternet) {
      anunta('Legarea merge doar acasă, pe Wi-Fi-ul casei', eroare: true);
      return false;
    }
    try {
      final r = await _api.imperecheaza(adresa, cod.replaceAll(' ', ''));
      if (r == null) {
        anunta(
          'Codul nu e bun sau a expirat. Cereți altul pe panou.',
          eroare: true,
        );
        return false;
      }
      final (s, cheie) = r;
      await _aflaServerul(s);
      _cheie = cheie;
      await _seif.write(key: 'cheie', value: base64.encode(cheie));
      notifyListeners();
      anunta('Telefonul e legat de casă');
      return true;
    } catch (_) {
      anunta('Panoul nu a putut fi contactat', eroare: true);
      return false;
    }
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
      } else if (_peInternet && _broker != null) {
        await _broker!.comanda(c, cheie: _cheie);
      } else {
        if (adresa.isEmpty) throw StateError('fara adresa');
        await _api.comanda(adresa, c);
      }
    } on StateError catch (e) {
      ok = false;
      anunta(
        e.message == 'fara cheie'
            ? 'Ca să dezarmați prin internet, legați întâi telefonul de casă (o dată, acasă)'
            : 'Panoul nu a putut fi contactat',
        eroare: true,
      );
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
      final nume = numeBiometrie[0].toUpperCase() + numeBiometrie.substring(1);
      anunta('$nume nu a mers. Folosiți PIN-ul.', eroare: true);
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
    WidgetsBinding.instance.removeObserver(this);
    _ceas?.cancel();
    _opresteBroker();
    super.dispose();
  }
}
