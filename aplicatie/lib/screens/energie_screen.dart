// Energie - cat din consumul de acum vine din soare si vant, bateria si productia pe ore
import 'dart:math';

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../models/api_models.dart';
import '../providers/app_state.dart';
import '../theme/nepha_theme.dart';
import '../widgets/grafice.dart';
import '../widgets/ui.dart';

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

    return Ecran(
      titlu: 'Energie',
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
                  'Următoarele 6 ore: ~${nr(en.estimatSolar6h)} Wh din soare',
                ),
              ],
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
        iconita: Icons.bar_chart_rounded,
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
          ? GraficEnergie(
              widget.app.istoric3,
              key: const ValueKey('trei'),
              zile: true,
              minutAcum: widget.app.stare?.minut ?? 0,
            )
          : GraficEnergie(
              widget.app.istoric,
              key: const ValueKey('azi'),
              minutAcum: widget.app.stare?.minut ?? 0,
            ),
    ],
  );
}
