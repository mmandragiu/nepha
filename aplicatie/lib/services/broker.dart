// broker - casa prin internet: cand telefonul nu e in Wi-Fi-ul casei, vorbim cu panoul prin
// brokerul MQTT (vezi panou/mqtt.ino). Mesajele au acelasi JSON ca pe Wi-Fi.
//
//   TOPIC_CERERI   noi intrebam:          {"id","client","server","cere":"stare|jurnal|istoric|rutina|comanda|privesc"}
//   TOPIC_RASPUNS  panoul ne raspunde:    {"id","ok","date"}           (doar noua, dupa id-ul telefonului)
//   TOPIC_NOUTATI  panoul anunta pe toti: {"tip":"stare|eveniment|online|offline","server","date"}
//
// Utilizatorul si parola nu sunt in aplicatie: fiecare telefon isi primeste contul lui pe broker
// de la panou, cand il legam de casa (codul de pe ecran), si il tine criptat pe telefon
// (flutter_secure_storage: Keychain pe iPhone, Keystore pe Android). Fara cont, doar Wi-Fi.
import 'dart:async';
import 'dart:convert';
import 'dart:math';

import 'package:cryptography/cryptography.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

import '../models/api_models.dart';

const brokerServer = 'mqtt.mandragiu.eu';
const brokerPort = 8883;
// la fel ca in panou/config.h: {server} = numele panoului, {client} = id-ul telefonului
const topicCereri = 'nepha/servers/{server}/requests';
const topicRaspuns = 'nepha/servers/{server}/clients/{client}/responses';
const topicNoutati = 'nepha/servers/{server}/events/devices';

String _topic(String model, {String server = '', String client = ''}) =>
    model.replaceAll('{server}', server).replaceAll('{client}', client);

// "ALARMA OPRIT 1234" -> secret pentru panou: AES-256-GCM cu cheia casei (de la imperechere),
// cu ora trimiterii inauntru (panoul refuza mesajele vechi sau retrimise)
Future<String> cripteazaComanda(
  List<int> cheie,
  String server,
  String comanda,
) async {
  final aes = AesGcm.with256bits();
  final acum = DateTime.now().millisecondsSinceEpoch ~/ 1000;
  final cutie = await aes.encrypt(
    utf8.encode('$acum $comanda'),
    secretKey: SecretKey(cheie),
    nonce: aes.newNonce(),
    aad: utf8.encode(server),
  );
  return base64.encode([
    ...cutie.nonce,
    ...cutie.cipherText,
    ...cutie.mac.bytes,
  ]);
}

class Broker {
  final String client; // id-ul telefonului, acelasi la fiecare pornire
  final String server; // numele panoului (adresa lui MAC, ex. a0b1c2d3e4f5)
  final String utilizator,
      parola; // contul telefonului pe broker, primit la legare
  final void Function(Stare) laStare;
  final void Function(Decizie) laEveniment;
  final void Function(bool) laPanou; // panoul a intrat / a iesit de pe broker

  Broker({
    required this.client,
    required this.server,
    required this.utilizator,
    required this.parola,
    required this.laStare,
    required this.laEveniment,
    required this.laPanou,
  });

  MqttServerClient? _mqtt;
  StreamSubscription<List<MqttReceivedMessage<MqttMessage>>>? _ascult;
  final _asteptam = <String, Completer<dynamic>>{};
  final _intamplare = Random.secure();
  int _numar = 0;

  bool get conectat =>
      _mqtt?.connectionStatus?.state == MqttConnectionState.connected;

  // brokerul nu ne mai primeste contul (sters sau resetat pe serverul de conturi): trebuie
  // legat telefonul din nou, ca panoul sa-i faca altul
  bool contRespins = false;

  Future<bool> porneste() async {
    if (conectat) return true;
    final m = MqttServerClient.withPort(brokerServer, client, brokerPort)
      ..secure = true
      ..keepAlivePeriod = 30
      ..autoReconnect = true
      ..connectTimeoutPeriod = 5000
      ..logging(on: false);
    _mqtt = m;
    try {
      await m.connect(utilizator, parola);
    } catch (_) {
      final cod = m.connectionStatus?.returnCode;
      contRespins =
          cod == MqttConnectReturnCode.badUsernameOrPassword ||
          cod == MqttConnectReturnCode.notAuthorized;
      m.disconnect();
      return false;
    }
    if (!conectat) return false;
    m.subscribe(
      _topic(topicRaspuns, server: server, client: client),
      MqttQos.atMostOnce,
    );
    m.subscribe(_topic(topicNoutati, server: server), MqttQos.atMostOnce);
    _ascult = m.updates?.listen(_primit);
    return true;
  }

  void opreste() {
    _ascult?.cancel();
    _ascult = null;
    _mqtt?.disconnect();
    _mqtt = null;
    for (final c in _asteptam.values) {
      c.completeError(StateError('oprit'));
    }
    _asteptam.clear();
  }

  void _primit(List<MqttReceivedMessage<MqttMessage>> mesaje) {
    for (final m in mesaje) {
      final p = m.payload;
      if (p is! MqttPublishMessage) continue;
      final Object? j;
      try {
        j = jsonDecode(utf8.decode(p.payload.message));
      } catch (_) {
        continue;
      }
      if (j is! Map<String, dynamic>) continue;
      if (j.containsKey('tip')) {
        // noutati: doar de la panoul nostru (pe broker pot fi mai multe case)
        if (j['server'] != server) continue;
        final date = j['date'];
        switch (j['tip']) {
          case 'stare' when date is Map<String, dynamic>:
            laStare(Stare.fromJson(date));
          case 'eveniment' when date is Map<String, dynamic>:
            laEveniment(Decizie.fromJson(date));
          case 'online':
            laPanou(true);
          case 'offline':
            laPanou(false);
        }
      } else {
        final c = _asteptam.remove(j['id']);
        if (c == null) continue;
        if (j['ok'] == true) {
          c.complete(j['date']);
        } else {
          c.completeError(StateError('${j['eroare'] ?? 'refuzat'}'));
        }
      }
    }
  }

  // o cerere si raspunsul ei; daca panoul nu raspunde in 6 s, renuntam
  Future<dynamic> _cere(String ce, [Map<String, Object?> inPlus = const {}]) {
    final m = _mqtt;
    if (m == null || !conectat) return Future.error(StateError('fara broker'));
    final id = '${_numar++}${_intamplare.nextInt(1 << 20).toRadixString(36)}';
    final c = Completer<dynamic>();
    _asteptam[id] = c;
    final b = MqttClientPayloadBuilder()
      ..addUTF8String(
        jsonEncode({
          'id': id,
          'client': client,
          'server': server,
          'cere': ce,
          ...inPlus,
        }),
      );
    m.publishMessage(
      _topic(topicCereri, server: server),
      MqttQos.atMostOnce,
      b.payload!,
    );
    return c.future.timeout(
      const Duration(seconds: 6),
      onTimeout: () {
        _asteptam.remove(id);
        throw TimeoutException('panoul nu raspunde');
      },
    );
  }

  // "ma uit": cat timp intrebam asa macar o data pe minut, panoul ne anunta ce se schimba
  Future<void> privesc() => _cere('privesc');

  Future<Stare> stare() async =>
      Stare.fromJson((await _cere('stare')) as Map<String, dynamic>);

  Future<List<Decizie>> jurnal() async => ((await _cere('jurnal')) as List)
      .map((e) => Decizie.fromJson(e as Map<String, dynamic>))
      .toList();

  Future<Istoric> istoric() async =>
      Istoric.fromJson((await _cere('istoric')) as Map<String, dynamic>);

  Future<Rutina> rutina() async =>
      Rutina.fromJson((await _cere('rutina')) as Map<String, dynamic>);

  // comenzile cu PIN pleaca doar criptate (panoul le refuza altfel)
  Future<void> comanda(String c, {List<int>? cheie}) async {
    if (c.startsWith('ALARMA OPRIT')) {
      if (cheie == null) throw StateError('fara cheie');
      await _cere('comanda', {
        'secret': await cripteazaComanda(cheie, server, c),
      });
    } else {
      await _cere('comanda', {'c': c});
    }
  }
}
