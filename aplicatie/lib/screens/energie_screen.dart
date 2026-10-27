// Energie - cat din consumul de acum vine din soare si vant, bateria, graficul zilei si
// "ziua de test" (clasic vs smart)
import 'dart:math';

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../models/api_models.dart';
import '../providers/app_state.dart';
import '../theme/nepha_theme.dart';
import '../widgets/grafice.dart';
import '../widgets/ui.dart';

const _explicatie = [
  'Energie destulă. Casa merge normal.',
  'Bateria e plină și panoul încă produce: pornim acum ce poate aștepta.',
  'Bateria scade și nu se produce: lumini mai slabe, un grad mai puțin.',
  'Bateria e aproape goală: doar strictul necesar.',
];
const _tonMod = [Ton.verde, Ton.soare, Ton.cald, Ton.rosu];

class EnergieScreen extends StatelessWidget {
  const EnergieScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final app = context.watch<AppState>();
    final s = app.stare;
    if (s == null) {
      return const Ecran(
        titlu: 'Energie',
        copii: [T.mic('Se așteaptă datele de la casă...')],
      );
    }

    final en = s.energie;
    final mod = en.modCod.clamp(0, 3);
    final culoareBaterie = [
      Culori.verde,
      Culori.verde,
      Culori.soare,
      Culori.rosu,
    ][mod];
    final avemTest =
        s.rezultate.length == 2 && s.rezultate[0].valid && s.rezultate[1].valid;

    return Ecran(
      titlu: 'Energie',
      subtitlu: 'la scara machetei, în wați',
      copii: [
        _DinSoare(en),
        // bateria
        Bloc(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Eticheta(
                'Baterie',
                iconita: Icons.battery_charging_full_outlined,
                dreapta: Pastila(
                  'Mod ${en.mod.toLowerCase()}',
                  ton: _tonMod[mod],
                ),
              ),
              Row(
                crossAxisAlignment: CrossAxisAlignment.baseline,
                textBaseline: TextBaseline.alphabetic,
                children: [
                  T.mare('${en.baterie}', marime: 52),
                  const T('%', marime: 20, gros: FontWeight.w600),
                ],
              ),
              const SizedBox(height: 12),
              BaraCasute(en.baterie, culoare: culoareBaterie),
              const SizedBox(height: 12),
              T.mic(_explicatie[mod]),
            ],
          ),
        ),
        Bloc(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              _Grafic(app),
              if (en.estimatSolar6h != null) ...[
                const SizedBox(height: 8),
                T.mic(
                  'În următoarele 6 ore casa estimează ~${nr(en.estimatSolar6h)} Wh din soare.',
                ),
              ],
            ],
          ),
        ),
        Bloc(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Eticheta('Ziua de test', iconita: Icons.balance_outlined),
              const T.mic(
                'Aceeași zi, trăită o dată ca o casă clasică și o dată ca o casă NEPHA. Aceeași familie, aceeași vreme.',
              ),
              const SizedBox(height: 14),
              if (avemTest)
                _Rezultate(s.rezultate[0], s.rezultate[1])
              else
                const T.mic(
                  'Încă n-am rulat ziua de test. Apăsați „Calculează”.',
                ),
              const SizedBox(height: 16),
              Row(
                children: [
                  Expanded(
                    child: Buton(
                      'Calculează',
                      principal: true,
                      onTap: () => app.comanda('TEST RAPID'),
                    ),
                  ),
                  const SizedBox(width: 8),
                  Expanded(
                    child: Buton(
                      s.test == 1 ? 'Oprește testul' : 'Pe machetă',
                      onTap: () =>
                          app.comanda(s.test == 1 ? 'TEST STOP' : 'TEST SMART'),
                    ),
                  ),
                ],
              ),
            ],
          ),
        ),
      ],
    );
  }
}

// semicercul: cat din consumul de acum il acopera soarele si vantul; restul vine din baterie
class _DinSoare extends StatelessWidget {
  final Energie en;
  const _DinSoare(this.en);

  @override
  Widget build(BuildContext context) {
    final productie = en.solar + en.vant;
    final procent = en.consum <= 0
        ? 100
        : (productie / en.consum * 100).clamp(0, 100).round();
    final dinBaterie = max(0.0, en.consum - productie);
    return Bloc(
      child: Column(
        children: [
          const Eticheta('Acum', iconita: Icons.bolt_outlined),
          SizedBox(
            width: 280,
            height: 150,
            child: Stack(
              alignment: Alignment.bottomCenter,
              children: [
                Positioned.fill(
                  child: TweenAnimationBuilder<double>(
                    tween: Tween(end: procent / 100),
                    duration: const Duration(milliseconds: 600),
                    curve: usor,
                    builder: (_, v, _) =>
                        CustomPaint(painter: _Semicerc(v, Culori.verde)),
                  ),
                ),
                Padding(
                  padding: const EdgeInsets.only(bottom: 4),
                  child: Column(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      const T.mic('din soare și vânt'),
                      Row(
                        mainAxisSize: MainAxisSize.min,
                        crossAxisAlignment: CrossAxisAlignment.baseline,
                        textBaseline: TextBaseline.alphabetic,
                        children: [
                          T.mare('$procent', marime: 48),
                          const T('%', marime: 18, gros: FontWeight.w600),
                        ],
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),
          const SizedBox(height: 16),
          Row(
            children: [
              Expanded(
                child: _Cifra(
                  'Producție',
                  '${nr(productie, 2)} W',
                  'soare ${nr(en.solar, 2)} · vânt ${nr(en.vant, 2)}',
                  Culori.verde,
                ),
              ),
              const SizedBox(width: 10),
              Expanded(
                child: _Cifra(
                  'Din baterie',
                  '${nr(dinBaterie, 2)} W',
                  'consum ${nr(en.consum, 2)} W',
                  Culori.moale,
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }
}

class _Semicerc extends CustomPainter {
  final double p;
  final Color c;
  _Semicerc(this.p, this.c);

  @override
  void paint(Canvas canvas, Size s) {
    const g = 16.0;
    final raza = min(s.width / 2, s.height) - g / 2;
    final centru = Offset(s.width / 2, s.height - 2);
    final r = Rect.fromCircle(center: centru, radius: raza);
    final linie = Paint()
      ..style = PaintingStyle.stroke
      ..strokeCap = StrokeCap.round
      ..strokeWidth = g;
    canvas.drawArc(r, pi, pi, false, linie..color = Culori.moale);
    if (p > 0) canvas.drawArc(r, pi, pi * p, false, linie..color = c);
  }

  @override
  bool shouldRepaint(_Semicerc vechi) => vechi.p != p || vechi.c != c;
}

// o cifra cu bulina ei de culoare (ca legenda semicercului)
class _Cifra extends StatelessWidget {
  final String eticheta, valoare, detaliu;
  final Color culoare;
  const _Cifra(this.eticheta, this.valoare, this.detaliu, this.culoare);

  @override
  Widget build(BuildContext context) => Container(
    padding: const EdgeInsets.all(14),
    decoration: BoxDecoration(
      color: Culori.moale,
      borderRadius: BorderRadius.circular(coltMic),
    ),
    child: Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          children: [
            Container(
              width: 10,
              height: 10,
              decoration: BoxDecoration(
                color: culoare == Culori.moale ? Culori.text2 : culoare,
                shape: BoxShape.circle,
              ),
            ),
            const SizedBox(width: 6),
            Expanded(child: T.mic(eticheta)),
          ],
        ),
        const SizedBox(height: 6),
        T(valoare, marime: 22, gros: FontWeight.w300),
        T.mic(detaliu, randuri: 1),
      ],
    ),
  );
}

class _Rezultate extends StatelessWidget {
  final RezultatTest c, s;
  const _Rezultate(this.c, this.s);

  int _pct(double a, double b) => a > 0 ? ((a - b) / a * 100).round() : 0;

  @override
  Widget build(BuildContext context) {
    final randuri = [
      ('Încălzire', c.incalzire, s.incalzire),
      ('Lumini', c.lumini, s.lumini),
      ('Priza TV (standby)', c.priza, s.priza),
      ('Udare', c.pompa, s.pompa),
    ];
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          crossAxisAlignment: CrossAxisAlignment.center,
          children: [
            T.mare('−${_pct(c.total, s.total)}%', culoare: Culori.verde),
            const SizedBox(width: 14),
            Expanded(
              child: T.mic(
                '${nr(c.total / 1000)} → ${nr(s.total / 1000)} kWh pe zi\n${nr(c.litri, 0)} → ${nr(s.litri, 0)} litri de apă',
              ),
            ),
          ],
        ),
        const SizedBox(height: 14),
        // fiecare consumator: casa clasica (cu dungi) deasupra, NEPHA dedesubt
        for (final (n, a, b) in randuri)
          Padding(
            padding: const EdgeInsets.only(bottom: 12),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  children: [
                    Expanded(child: T(n, marime: 13, gros: FontWeight.w600)),
                    T.mic('${nr(a, 0)} → ${nr(b, 0)} Wh  '),
                    T(
                      '−${_pct(a, b)}%',
                      marime: 13,
                      gros: FontWeight.w700,
                      culoare: Culori.verde,
                    ),
                  ],
                ),
                const SizedBox(height: 6),
                Bara(100, inaltime: 6, dungi: true, culoare: Culori.text2),
                const SizedBox(height: 4),
                Bara(
                  a > 0 ? b / a * 100 : 0,
                  inaltime: 6,
                  culoare: Culori.verde,
                ),
              ],
            ),
          ),
        Row(
          children: [
            SizedBox(
              width: 22,
              child: Bara(100, inaltime: 6, dungi: true, culoare: Culori.text2),
            ),
            const SizedBox(width: 6),
            const T.mic('casă clasică'),
            const SizedBox(width: 14),
            SizedBox(
              width: 22,
              child: Bara(100, inaltime: 6, culoare: Culori.verde),
            ),
            const SizedBox(width: 6),
            const T.mic('NEPHA'),
          ],
        ),
        if (c.mwh > 0) ...[
          const SizedBox(height: 10),
          T.mic(
            'Măsurat pe machetă (INA219): ${nr(c.mwh, 0)} → ${nr(s.mwh, 0)} mWh',
          ),
        ],
      ],
    );
  }
}

// graficul: ultimele 24 de ore (de la panou) sau ultimele 3 zile (tinute pe telefon)
class _Grafic extends StatefulWidget {
  final AppState app;
  const _Grafic(this.app);

  @override
  State<_Grafic> createState() => _GraficState();
}

class _GraficState extends State<_Grafic> {
  bool _trei = false;

  @override
  Widget build(BuildContext context) => Column(
    crossAxisAlignment: CrossAxisAlignment.start,
    children: [
      Eticheta(
        _trei ? 'Ultimele 3 zile' : 'Azi',
        iconita: Icons.show_chart_rounded,
        dreapta: widget.app.istoric3 == null
            ? null
            : SizedBox(
                width: 150,
                child: Alegere(
                  optiuni: const {'azi': 'Azi', 'trei': '3 zile'},
                  ales: _trei ? 'trei' : 'azi',
                  onAlege: (v) => setState(() => _trei = v == 'trei'),
                ),
              ),
      ),
      _trei
          ? GraficEnergie(widget.app.istoric3, zile: true)
          : GraficEnergie(widget.app.istoric),
    ],
  );
}
