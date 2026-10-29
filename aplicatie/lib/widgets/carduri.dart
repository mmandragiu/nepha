// cardurile din grila de pe Acasa. Pe scurt: iconita rotunda, o valoare mare, un rand mic si un
// semn tehnic discret (bulina verde = senzorul raspunde, galben = valoare estimata).
// Tinut apasat, cardul se mareste si arata "Date avansate": graficul ultimelor 24 de ore,
// de unde vine valoarea (senzor, pin, adresa I2C), drumul pana la telefon si cat a durat.
import 'dart:ui';

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../providers/app_state.dart';
import '../theme/nepha_theme.dart';
import 'ui.dart';

class DateCard {
  final String id;
  final IconData icon;
  final Color culoare;
  final String titlu, valoare, detaliu;
  final bool aprins; // e pornit / se intampla acum: iconita straluceste
  final (Color, String) semn; // bulina si textul mic din colt
  final List<double?>?
  grafic; // ultimele 24 de ore (null = n-avem istoric pentru asta)
  final String unitate; // pentru min / max de sub grafic
  final List<(String, String)> tehnic;
  final int? fila; // atingerea deschide fila asta
  const DateCard({
    required this.id,
    required this.icon,
    required this.culoare,
    required this.titlu,
    required this.valoare,
    required this.detaliu,
    this.aprins = false,
    required this.semn,
    this.grafic,
    this.unitate = '',
    required this.tehnic,
    this.fila,
  });

  // aprinse, iconitele sunt toate portocalii (accentul); doar pericolul ramane rosu
  Color? get aprinsCu => culoare == Culori.rosu ? Culori.rosu : null;
}

// bitii din "senzori": ce a gasit Arduino la pornire
const _bitCamere = 1,
    _bitAfara = 2,
    _bitSolar = 4,
    _bitVant = 8,
    _bitConsum = 16,
    _bitSol = 64;

List<DateCard> carduriCasa(AppState app) {
  final s = app.stare!;
  final i = app.istoric;
  final en = s.energie;
  final al = s.alarma;
  bool are(int bit) => s.senzori & bit != 0;
  final bun = s.arduinoVorbeste ? Culori.verde : Culori.soare;
  final deschise = al.senzori.where((x) => x.deschis).toList();
  final dht = s.camere.where((c) => c.umiditate != null).firstOrNull;
  final culoareMod = [
    Culori.verde,
    Culori.apa,
    Culori.soare,
    Culori.rosu,
  ][en.modCod.clamp(0, 3)];
  String onOff(bool b) => b ? 'ON' : 'OFF';

  return [
    DateCard(
      id: 'casa',
      icon: Icons.thermostat_outlined,
      culoare: Culori.cald,
      titlu: 'În casă',
      valoare: grade(s.tempCasa),
      detaliu: s.incalzire.pornita
          ? 'încălzește → ${grade(s.incalzire.tinta)}'
          : 'țintă ${grade(s.incalzire.tinta)}',
      aprins: s.incalzire.pornita,
      semn: dht != null
          ? (bun, 'DHT11')
          : are(_bitCamere)
          ? (bun, '1-Wire')
          : (Culori.soare, 'model'),
      grafic: i?.tCasa,
      unitate: '°',
      fila: 1,
      tehnic: [
        (
          'Senzori',
          dht != null
              ? 'DHT11 · D3 · ${dht.nume}'
              : are(_bitCamere)
              ? 'DS18B20 · 1-Wire · D4'
              : 'lipsă: model termic',
        ),
        if (dht != null) ('Umiditate aer', '${dht.umiditate}%'),
        ('Încălzire', 'MOSFET · D11 · ${onOff(s.incalzire.pornita)}'),
        ('De ce', '#${s.incalzire.motivCod} ${s.incalzire.motiv}'),
        ('Rată învățată', '${nr(s.incalzire.rata)} °C/h'),
        for (final c in s.camere) (c.nume, grade(c.temp)),
      ],
    ),
    DateCard(
      id: 'afara',
      icon: Icons.cloud_outlined,
      culoare: Culori.apa,
      titlu: 'Afară',
      valoare: grade(s.afara ?? (s.vreme.valida ? s.vreme.temp : null)),
      detaliu: !s.vreme.valida
          ? 'fără prognoză'
          : s.vreme.oraPloaie != null
          ? 'ploaie pe la ${s.vreme.oraPloaie}:00'
          : 'fără ploaie',
      semn: are(_bitAfara)
          ? (bun, '1-Wire')
          : s.vreme.valida
          ? (Culori.apa, 'meteo')
          : (Culori.text2, '—'),
      grafic: i?.tAfara,
      unitate: '°',
      tehnic: [
        (
          'Senzor',
          are(_bitAfara) ? 'DS18B20 · capsulă · D4' : 'lipsă: din prognoză',
        ),
        ('Lumină naturală', 'fotorezistență · A1 · ${s.luminaNaturala}%'),
        (
          'Prognoză',
          s.vreme.valida
              ? 'Open-Meteo · HTTPS · ESP32'
              : 'panoul n-are internet',
        ),
        if (s.vreme.valida) ...[
          ('Ploaie', '${s.vreme.ploaie ?? 0}%'),
          ('Nori · soare', '${s.vreme.nori ?? 0}% · ${s.vreme.soare ?? 0}%'),
          ('Vânt', '${s.vreme.vant ?? 0} km/h'),
        ],
      ],
    ),
    DateCard(
      id: 'baterie',
      icon: Icons.battery_charging_full_outlined,
      culoare: culoareMod,
      titlu: 'Baterie',
      valoare: '${en.baterie}%',
      detaliu: 'mod ${en.mod.toLowerCase()}',
      aprins: en.modCod >= 2,
      semn: are(_bitConsum) ? (bun, 'INA219') : (Culori.soare, 'estimat'),
      grafic: i?.baterie.map((b) => b.toDouble()).toList(),
      unitate: '%',
      fila: 2,
      tehnic: [
        ('Măsurare', 'INA219 · I2C 0x44'),
        ('Mod energie', '#${en.modCod} ${en.mod}'),
        if (en.estimatSolar6h != null)
          ('Soare în 6 h', '~${nr(en.estimatSolar6h)} Wh'),
        ('Factor panou', nr(en.factorPanou, 2)),
      ],
    ),
    DateCard(
      id: 'productie',
      icon: Icons.solar_power_outlined,
      culoare: Culori.soare,
      titlu: 'Producție',
      valoare: '${nr(en.solar + en.vant, 2)} W',
      detaliu: 'soare ${nr(en.solar, 2)} · vânt ${nr(en.vant, 2)}',
      aprins: en.solar + en.vant > 0.05,
      semn: are(_bitSolar) ? (bun, 'INA219') : (Culori.soare, 'estimat'),
      grafic: i == null
          ? null
          : [
              for (var k = 0; k < i.solar.length; k++)
                i.solar[k] + (k < i.vant.length ? i.vant[k] : 0),
            ],
      unitate: ' W',
      fila: 2,
      tehnic: [
        (
          'Soare',
          are(_bitSolar)
              ? 'INA219 · I2C 0x40 · ${nr(en.solar, 3)} W'
              : 'din fotorezistență · ${nr(en.solar, 3)} W',
        ),
        (
          'Vânt',
          are(_bitVant) ? 'INA219 · I2C 0x41 · ${nr(en.vant, 3)} W' : 'lipsă',
        ),
        ('Scară', 'machetă (wați mici)'),
      ],
    ),
    DateCard(
      id: 'consum',
      icon: Icons.electric_bolt_outlined,
      culoare: Culori.apa,
      titlu: 'Consum',
      valoare: '${nr(en.consum, 2)} W',
      detaliu: s.priza ? 'priza TV pornită' : 'priza TV oprită',
      semn: are(_bitConsum) ? (bun, 'INA219') : (Culori.soare, 'estimat'),
      grafic: i?.consum,
      unitate: ' W',
      fila: 2,
      tehnic: [
        ('Măsurare', 'INA219 · I2C 0x44'),
        ('Priză TV', 'releu · D12 · ${onOff(s.priza)}'),
        ('Măsurat azi', '${nr(en.mwh, 0)} mWh'),
      ],
    ),
    DateCard(
      id: 'alarma',
      icon: Icons.shield_outlined,
      culoare: al.pericol
          ? Culori.rosu
          : (al.cod == 0 ? Culori.text2 : Culori.verde),
      titlu: 'Alarmă',
      valoare: al.pericol ? 'ALERTĂ' : (al.cod == 0 ? 'Oprită' : 'Armată'),
      detaliu: deschise.isEmpty
          ? 'totul e închis'
          : '${deschise.length} ${deschise.length == 1 ? 'deschis' : 'deschise'}',
      aprins: al.cod >= 2,
      semn: (deschise.isEmpty ? bun : Culori.cald, '433 MHz'),
      fila: 3,
      tehnic: [
        ('Receptor', 'SYN480R · 433 MHz · D2'),
        ('Codare', 'EV1527 · RCSwitch'),
        ('Sirenă', 'buzzer · D13'),
        ('Stare', '#${al.cod} ${al.stare}'),
        for (final u in al.senzori) (u.nume, u.deschis ? 'DESCHIS' : 'închis'),
      ],
    ),
    DateCard(
      id: 'gradina',
      icon: Icons.water_drop_outlined,
      culoare: Culori.apa,
      titlu: 'Grădină',
      valoare: '${s.gradina.umiditate}%',
      detaliu: s.gradina.zona > 0
          ? 'zona ${s.gradina.zona} din 4'
          : s.gradina.pompa
          ? 'se udă acum'
          : s.gradina.ploua == true
          ? 'plouă acum'
          : s.gradina.umiditate >= 35
          ? 'pământ umed'
          : 'pământ uscat',
      aprins: s.gradina.pompa || s.gradina.zona > 0,
      semn: are(_bitSol) ? (bun, 'A0') : (Culori.soare, 'model'),
      fila: 1,
      tehnic: [
        (
          'Pământ',
          are(_bitSol)
              ? 'capacitiv · A0 analog'
              : 'calculat: soare, udat, ploaie',
        ),
        ('Pompă', 'MOSFET · D10 · ${onOff(s.gradina.pompa)}'),
        (
          'Placa din grădină',
          s.gradina.ploua == null ? 'nu răspunde' : 'ESP32-S3 · ESP-NOW',
        ),
        if (s.gradina.ploua != null)
          (
            'Senzor ploaie',
            'LM393 · GPIO4 · ${s.gradina.ploua! ? 'ud' : 'uscat'}',
          ),
        if (s.gradina.ploua != null)
          (
            'Electrovalve',
            '4 relee · ${s.gradina.zona > 0 ? 'zona ${s.gradina.zona}' : 'închise'}',
          ),
        if (s.vreme.oraPloaie != null)
          ('Ploaie', '${s.vreme.ploaie}% pe la ${s.vreme.oraPloaie}:00'),
      ],
    ),
  ];
}

// drumul pana la casa: acelasi pentru toate cardurile
List<(String, String, Color?)> _drum(AppState app) {
  final s = app.stare!;
  final c = app.conexiune;
  final ms = app.latentaMs;
  final secunde = app.stareLa == null
      ? null
      : DateTime.now().difference(app.stareLa!).inSeconds;
  return [
    (
      'Drum',
      switch (c) {
        Conexiune.demo => 'demo · fără panou',
        Conexiune.internet => 'MQTT · TLS · mqtt.mandragiu.eu:8883',
        _ =>
          s.retea.ip.isEmpty
              ? 'Wi-Fi · HTTP · ${app.adresa}'
              : 'Wi-Fi · HTTP · ${s.retea.ip}',
      },
      null,
    ),
    if (s.server.isNotEmpty) ('Panou', 'ESP32 · ${s.server}', null),
    (
      'Latență',
      ms == null ? '—' : '$ms ms',
      ms == null
          ? null
          : (ms < 150 ? Culori.verde : (ms < 600 ? Culori.soare : Culori.cald)),
    ),
    ('Actualizat', secunde == null ? '—' : 'acum $secunde s', null),
    (
      'Arduino → panou',
      s.vechime < 0 ? 'încă nimic' : 'I2C 0x08 · acum ${s.vechime} s',
      s.arduinoVorbeste ? null : Culori.cald,
    ),
  ];
}

// ---------------------------------------------------------------- grila

// carduri pe doua coloane, cu aceeasi inaltime pe un rand
class GrilaCarduri extends StatelessWidget {
  final List<DateCard> carduri;
  final void Function(int fila) mergiLa;
  const GrilaCarduri(this.carduri, {super.key, required this.mergiLa});

  @override
  Widget build(BuildContext context) => Column(
    children: [
      for (var k = 0; k < carduri.length; k += 2)
        Padding(
          padding: EdgeInsets.only(top: k == 0 ? 0 : 12),
          child: IntrinsicHeight(
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                // un card ramas singur pe ultimul rand ia toata latimea
                Expanded(child: CardMic(carduri[k], mergiLa: mergiLa)),
                if (k + 1 < carduri.length) ...[
                  const SizedBox(width: 12),
                  Expanded(child: CardMic(carduri[k + 1], mergiLa: mergiLa)),
                ],
              ],
            ),
          ),
        ),
    ],
  );
}

class CardMic extends StatelessWidget {
  final DateCard d;
  final void Function(int fila) mergiLa;
  const CardMic(this.d, {super.key, required this.mergiLa});

  @override
  Widget build(BuildContext context) => Hero(
    tag: 'card:${d.id}',
    // in zbor aratam doar cardul gol; textul apare cand s-a deschis fereastra
    flightShuttleBuilder: (_, _, _, _, _) => Container(decoration: decorCard()),
    child: Bloc(
      padding: const EdgeInsets.all(16),
      onTap: d.fila == null ? null : () => mergiLa(d.fila!),
      onLongPress: () => arataDateAvansate(context, d.id),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Iconita(d.icon, culoare: d.aprinsCu, aprinsa: d.aprins),
              const SizedBox(width: 10),
              Expanded(
                child: FittedBox(
                  fit: BoxFit.scaleDown,
                  alignment: Alignment.centerRight,
                  child: T(d.valoare, marime: 30, gros: FontWeight.w300),
                ),
              ),
            ],
          ),
          const SizedBox(height: 14),
          T(d.titlu, marime: 15, gros: FontWeight.w700, randuri: 1),
          const SizedBox(height: 2),
          T.mic(d.detaliu, randuri: 1),
          const Spacer(),
          const SizedBox(height: 10),
          Row(
            children: [
              Punct(culoare: d.semn.$1, marime: 6),
              const SizedBox(width: 6),
              Expanded(
                child: T(
                  d.semn.$2,
                  marime: 11,
                  culoare: Culori.text2,
                  randuri: 1,
                ),
              ),
            ],
          ),
        ],
      ),
    ),
  );
}

// ---------------------------------------------------------------- "Date avansate"

void arataDateAvansate(BuildContext context, String id) {
  Navigator.of(context).push(
    PageRouteBuilder<void>(
      opaque: false,
      barrierDismissible: true,
      barrierColor: Colors.transparent,
      transitionDuration: const Duration(milliseconds: 420),
      reverseTransitionDuration: const Duration(milliseconds: 320),
      pageBuilder: (_, a, _) => _DateAvansate(id, a),
    ),
  );
}

class _DateAvansate extends StatelessWidget {
  final String id;
  final Animation<double> a;
  const _DateAvansate(this.id, this.a);

  @override
  Widget build(BuildContext context) {
    // se reface la fiecare stare noua: cifrele se misca si cu fereastra deschisa
    final app = context.watch<AppState>();
    if (app.stare == null) return const SizedBox();
    final d = carduriCasa(app).firstWhere((c) => c.id == id);
    final curba = CurvedAnimation(parent: a, curve: usor);
    final continut = CurvedAnimation(
      parent: a,
      curve: const Interval(0.45, 1, curve: Curves.easeOut),
    );
    final g = d.grafic?.whereType<double>().toList() ?? const <double>[];

    return GestureDetector(
      onTap: () => Navigator.of(context).pop(),
      child: AnimatedBuilder(
        animation: curba,
        builder: (_, child) => BackdropFilter(
          filter: ImageFilter.blur(
            sigmaX: 14 * curba.value,
            sigmaY: 14 * curba.value,
          ),
          child: Container(
            color: Colors.black.withValues(
              alpha: (Culori.intunecat ? 0.5 : 0.25) * curba.value,
            ),
            child: child,
          ),
        ),
        child: SafeArea(
          child: Center(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: GestureDetector(
                onTap: () {}, // atingerea pe card nu inchide
                child: Hero(
                  tag: 'card:${d.id}',
                  flightShuttleBuilder: (_, _, _, _, _) =>
                      Container(decoration: decorCard()),
                  child: Material(
                    type: MaterialType.transparency,
                    child: ConstrainedBox(
                      constraints: const BoxConstraints(maxWidth: 480),
                      child: Container(
                        decoration: decorCard(),
                        child: FadeTransition(
                          opacity: continut,
                          child: SingleChildScrollView(
                            padding: const EdgeInsets.all(20),
                            child: _Continut(d, g, _drum(app)),
                          ),
                        ),
                      ),
                    ),
                  ),
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }
}

class _Continut extends StatelessWidget {
  final DateCard d;
  final List<double> g;
  final List<(String, String, Color?)> drum;
  const _Continut(this.d, this.g, this.drum);

  @override
  Widget build(BuildContext context) => Column(
    mainAxisSize: MainAxisSize.min,
    crossAxisAlignment: CrossAxisAlignment.start,
    children: [
      Row(
        children: [
          Iconita(d.icon, culoare: d.aprinsCu, aprinsa: d.aprins, marime: 44),
          const SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                T(d.titlu, marime: 18, gros: FontWeight.w700),
                T.mic('Date avansate', culoare: Culori.accent),
              ],
            ),
          ),
          ButonRotund(
            Icons.close_rounded,
            eticheta: 'Închide',
            onTap: () => Navigator.of(context).pop(),
          ),
        ],
      ),
      const SizedBox(height: 16),
      Row(
        crossAxisAlignment: CrossAxisAlignment.end,
        children: [
          Flexible(child: T.mare(d.valoare)),
          const SizedBox(width: 12),
          Expanded(
            child: Padding(
              padding: const EdgeInsets.only(bottom: 6),
              child: T.mic(d.detaliu),
            ),
          ),
        ],
      ),
      if (d.grafic != null) ...[
        const SizedBox(height: 18),
        Row(
          children: [
            const Expanded(
              child: T('Ultimele 24 de ore', marime: 13, gros: FontWeight.w700),
            ),
            if (g.length >= 2)
              T.mic(
                'min ${nr(g.reduce((a, b) => a < b ? a : b))}${d.unitate}  ·  max ${nr(g.reduce((a, b) => a > b ? a : b))}${d.unitate}',
              ),
          ],
        ),
        const SizedBox(height: 8),
        Sparkline(d.grafic!, culoare: d.culoare, inaltime: 84),
      ],
      const SizedBox(height: 16),
      const T('De unde vine', marime: 13, gros: FontWeight.w700),
      const SizedBox(height: 4),
      for (final (e, v) in d.tehnic) RandTehnic(e, v),
      const SizedBox(height: 12),
      const T('Legătura', marime: 13, gros: FontWeight.w700),
      const SizedBox(height: 4),
      for (final (e, v, c) in drum) RandTehnic(e, v, culoare: c),
    ],
  );
}
