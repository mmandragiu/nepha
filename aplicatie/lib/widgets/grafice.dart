// grafice - productia pe ore (bare) si harta rutinei
import 'dart:math';

import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart' show OverflowBoxFit;

import '../models/api_models.dart';
import '../theme/nepha_theme.dart';
import 'ui.dart';

const _zileSapt = ['lun', 'mar', 'mie', 'joi', 'vin', 'sâm', 'dum'];

// graficul productiei (soare + vant): o bara pe ora, iar la 3 zile o bara la 3 ore.
// Atingi sau tragi cu degetul peste bare si vezi cat s-a produs atunci; la inceput
// e aleasa ora de acum. Orele care n-au venit inca raman gri.
class GraficEnergie extends StatefulWidget {
  final Istoric? istoric;
  // zile = true: `minut` e minutul de la 1970 (graficul pe 3 zile de pe telefon), altfel minutul zilei
  final bool zile;
  final int minutAcum; // minutul zilei la casa
  const GraficEnergie(
    this.istoric, {
    super.key,
    this.zile = false,
    required this.minutAcum,
  });

  @override
  State<GraficEnergie> createState() => _GraficEnergieState();
}

class _GraficEnergieState extends State<GraficEnergie> {
  int? _ales;

  // cate o bara: (ce scrie in bula, Wh; null = n-a venit inca / lipsesc punctele)
  List<(String, double?)> _bare(Istoric i) {
    double prod(int k) => i.solar[k] + (k < i.vant.length ? i.vant[k] : 0);
    final suma = List.filled(24, 0.0), cate = List.filled(24, 0);

    if (!widget.zile) {
      for (var k = 0; k < i.minut.length; k++) {
        // panoul tine 24 de ore: ce e "mai tarziu" decat acum e de ieri
        if (i.minut[k] > widget.minutAcum + 5) continue;
        final h = (i.minut[k] ~/ 60).clamp(0, 23);
        suma[h] += prod(k);
        cate[h]++;
      }
      final oraAcum = widget.minutAcum ~/ 60;
      // media in wati pe o ora = Wh in ora aceea
      return [
        for (var h = 0; h < 24; h++)
          ('$h:00', h > oraAcum || cate[h] == 0 ? null : suma[h] / cate[h]),
      ];
    }

    // 3 zile: 24 de bucati a cate 3 ore, ultima e cea de acum
    final azi = DateTime.now();
    final ultima = DateTime(
      azi.year,
      azi.month,
      azi.day,
      azi.hour - azi.hour % 3,
    );
    final prima = ultima.subtract(const Duration(hours: 69));
    final m0 = prima.millisecondsSinceEpoch ~/ 60000;
    for (var k = 0; k < i.minut.length; k++) {
      if (i.minut[k] < m0) continue;
      final b = (i.minut[k] - m0) ~/ 180;
      if (b > 23) continue;
      suma[b] += prod(k);
      cate[b]++;
    }
    return [
      for (var b = 0; b < 24; b++)
        (
          '${_zileSapt[prima.add(Duration(hours: 3 * b)).weekday - 1]} '
              '${prima.add(Duration(hours: 3 * b)).hour}:00',
          cate[b] == 0 ? null : suma[b] / cate[b] * 3,
        ),
    ];
  }

  void _alege(Offset p, double latime, List<(String, double?)> bare) {
    final b = (p.dx / latime * bare.length).floor().clamp(0, bare.length - 1);
    if (b == _ales || bare[b].$2 == null) return;
    atingere();
    setState(() => _ales = b);
  }

  @override
  Widget build(BuildContext context) {
    final i = widget.istoric;
    if (i == null || i.minut.length < 2) {
      return const SizedBox(
        height: 160,
        child: Center(child: T.mic('graficul apare după câteva minute')),
      );
    }
    final bare = _bare(i);
    final ultima = bare.lastIndexWhere((b) => b.$2 != null);
    if (ultima < 0) {
      return const SizedBox(
        height: 160,
        child: Center(child: T.mic('graficul apare după câteva minute')),
      );
    }
    final ales = _ales != null && bare[_ales!].$2 != null ? _ales! : ultima;
    final total = bare.fold(0.0, (a, b) => a + (b.$2 ?? 0));

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          crossAxisAlignment: CrossAxisAlignment.baseline,
          textBaseline: TextBaseline.alphabetic,
          children: [
            T.mare(nr(total), marime: 40),
            const SizedBox(width: 4),
            const T('Wh', marime: 16, gros: FontWeight.w600),
            const SizedBox(width: 8),
            T.mic(widget.zile ? 'produși în 3 zile' : 'produși azi'),
          ],
        ),
        const SizedBox(height: 10),
        LayoutBuilder(
          builder: (context, c) => GestureDetector(
            behavior: HitTestBehavior.opaque,
            onTapDown: (d) => _alege(d.localPosition, c.maxWidth, bare),
            onHorizontalDragUpdate: (d) =>
                _alege(d.localPosition, c.maxWidth, bare),
            child: CustomPaint(
              size: Size(c.maxWidth, 150),
              painter: _Bare(bare, ales: ales, zile: widget.zile),
            ),
          ),
        ),
      ],
    );
  }
}

class _Bare extends CustomPainter {
  final List<(String, double?)> bare;
  final int ales;
  final bool zile;
  // culorile le luam acum: daca se schimba tema, se redeseneaza
  final plin = Culori.verde, gol = Culori.moale, accent = Culori.accent;
  final text2 = Culori.text2,
      fundalBula = Culori.text,
      textBula = Culori.fundal;
  _Bare(this.bare, {required this.ales, required this.zile});

  static const _sus = 30.0, _jos = 18.0;

  TextPainter _scrie(String t, Color c, double marime, [FontWeight? g]) =>
      TextPainter(
        text: TextSpan(
          text: t,
          style: fontText(marime: marime, culoare: c, gros: g),
        ),
        textDirection: TextDirection.ltr,
      )..layout();

  @override
  void paint(Canvas canvas, Size s) {
    final n = bare.length;
    final loc = s.width / n;
    final lat = max(3.0, loc * 0.62);
    final inalt = s.height - _sus - _jos;
    final maxim = max(0.05, bare.map((b) => b.$2 ?? 0).reduce(max));
    double cat(double? v) =>
        v == null || v <= 0 ? 4.0 : max(4.0, v / maxim * inalt);
    final p = Paint();

    for (var b = 0; b < n; b++) {
      final v = bare[b].$2;
      final h = cat(v);
      final x = b * loc + (loc - lat) / 2;
      p.color = b == ales ? accent : (v == null || v <= 0 ? gol : plin);
      canvas.drawRRect(
        RRect.fromRectAndRadius(
          Rect.fromLTWH(x, _sus + inalt - h, lat, h),
          Radius.circular(min(lat / 2, 4)),
        ),
        p,
      );

      // jos: orele 0, 6, 12, 18 (la 3 zile: ziua, la miezul noptii)
      final eticheta = zile
          ? (bare[b].$1.endsWith(' 0:00') ? bare[b].$1.split(' ').first : null)
          : (b % 6 == 0 ? '$b' : null);
      if (eticheta != null) {
        _scrie(eticheta, text2, 10).paint(canvas, Offset(x, s.height - 13));
      }
    }

    // bula de deasupra barei alese: ora si cat s-a produs
    final v = bare[ales].$2 ?? 0;
    final tp = _scrie(
      '${bare[ales].$1} · ${nr(v, 2)} Wh',
      textBula,
      11,
      FontWeight.w600,
    );
    final lb = tp.width + 18;
    final cx = ales * loc + loc / 2;
    final y = max(0.0, _sus + inalt - cat(v) - 26);
    final st = (cx - lb / 2).clamp(0.0, max(0.0, s.width - lb)).toDouble();
    canvas.drawRRect(
      RRect.fromRectAndRadius(
        Rect.fromLTWH(st, y, lb, 20),
        const Radius.circular(10),
      ),
      p..color = fundalBula,
    );
    tp.paint(canvas, Offset(st + 9, y + 10 - tp.height / 2));
  }

  @override
  bool shouldRepaint(_Bare vechi) =>
      vechi.ales != ales ||
      vechi.bare != bare ||
      vechi.plin != plin ||
      vechi.fundalBula != fundalBula;
}

// ---------------------------------------------------------------- ce a invatat casa

class HartaRutina extends StatelessWidget {
  final Rutina? rutina;
  final int oraAcum;
  const HartaRutina(this.rutina, {super.key, required this.oraAcum});

  static const _nume = 72.0;

  @override
  Widget build(BuildContext context) {
    final r = rutina;
    if (r == null) return const T.mic('Se încarcă...');
    return Column(
      children: [
        for (var c = 0; c < r.camere.length && c < r.p.length; c++)
          Padding(
            padding: const EdgeInsets.only(bottom: 4),
            child: Row(
              children: [
                SizedBox(width: _nume, child: T.mic(r.camere[c], randuri: 1)),
                for (var h = 0; h < 24; h++)
                  Expanded(
                    child: Container(
                      height: 24,
                      margin: const EdgeInsets.symmetric(horizontal: 0.75),
                      decoration: BoxDecoration(
                        // cu cat e mai probabil sa fie cineva, cu atat mai portocaliu
                        color: Culori.accent.withValues(
                          alpha:
                              0.06 +
                              (max(
                                        0,
                                        (h < r.p[c].length ? r.p[c][h] : 0) -
                                            10,
                                      ) /
                                      90) *
                                  0.84,
                        ),
                        borderRadius: BorderRadius.circular(3),
                      ),
                      // ora de acum: un chenar desenat peste celula, fara sa-i ia din loc
                      foregroundDecoration: h == oraAcum
                          ? BoxDecoration(
                              borderRadius: BorderRadius.circular(3),
                              border: Border.all(
                                color: Culori.text,
                                width: 1.5,
                              ),
                            )
                          : null,
                    ),
                  ),
              ],
            ),
          ),
        // orele: 0, 6, 12, 18 (textul poate iesi peste coloanele vecine, nu se rupe).
        // Inaltimea o ia de la text (deferToChild): altfel, intr-o lista, OverflowBox s-ar
        // face infinit de inalt si ecranul s-ar derula la nesfarsit in gol
        Row(
          children: [
            const SizedBox(width: _nume),
            for (var h = 0; h < 24; h++)
              Expanded(
                child: OverflowBox(
                  fit: OverflowBoxFit.deferToChild,
                  minWidth: 0, // pe tableta coloana e mai lata de 40
                  maxWidth: 40,
                  alignment: Alignment.centerLeft,
                  child: Text(
                    h % 6 == 0 ? '$h' : '',
                    softWrap: false,
                    style: fontText(marime: 10, culoare: Culori.text2),
                  ),
                ),
              ),
          ],
        ),
      ],
    );
  }
}
