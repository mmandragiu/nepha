// grafice - graficul de energie (fl_chart, fara gradienti) si harta rutinei
import 'dart:math';

import 'package:fl_chart/fl_chart.dart';
import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart' show OverflowBoxFit;

import '../models/api_models.dart';
import '../theme/nepha_theme.dart';
import 'ui.dart';

// media pe cateva puncte vecine: caloriferul porneste si se opreste des, iar linia zimtata nu se citeste
List<double> _netezeste(List<double> v, [int k = 3]) => [
  for (var i = 0; i < v.length; i++)
    () {
      final b = v.sublist(max(0, i - k), min(v.length, i + k + 1));
      return b.reduce((a, x) => a + x) / b.length;
    }(),
];

class GraficEnergie extends StatelessWidget {
  final Istoric? istoric;
  const GraficEnergie(this.istoric, {super.key});

  @override
  Widget build(BuildContext context) {
    final i = istoric;
    if (i == null || i.minut.length < 2) {
      return const SizedBox(
        height: 160,
        child: Center(child: T.mic('graficul apare după câteva minute')),
      );
    }
    final n = i.minut.length;
    final prod = [
      for (var k = 0; k < n; k++)
        i.solar[k] + (k < i.vant.length ? i.vant[k] : 0),
    ];
    final consum = _netezeste(i.consum);
    final maxim = [0.5, ...prod, ...consum].reduce(max) * 1.15;
    List<FlSpot> puncte(List<double> v) => [
      for (var k = 0; k < v.length; k++) FlSpot(i.minut[k].toDouble(), v[k]),
    ];

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        SizedBox(
          height: 160,
          child: LineChart(
            LineChartData(
              minY: 0,
              maxY: maxim,
              minX: i.minut.first.toDouble(),
              maxX: i.minut.last.toDouble(),
              gridData: const FlGridData(show: false),
              borderData: FlBorderData(show: false),
              lineTouchData: const LineTouchData(enabled: false),
              titlesData: FlTitlesData(
                leftTitles: const AxisTitles(),
                rightTitles: const AxisTitles(),
                topTitles: const AxisTitles(),
                bottomTitles: AxisTitles(
                  sideTitles: SideTitles(
                    showTitles: true,
                    reservedSize: 20,
                    interval: 180, // din 3 in 3 ore
                    getTitlesWidget: (v, meta) {
                      if (v == meta.min || v == meta.max) {
                        return const SizedBox.shrink();
                      }
                      return SideTitleWidget(
                        meta: meta,
                        child: T.mic('${v ~/ 60}:00'),
                      );
                    },
                  ),
                ),
              ),
              lineBarsData: [
                LineChartBarData(
                  spots: puncte(prod),
                  color: Culori.soare,
                  barWidth: 2,
                  dotData: const FlDotData(show: false),
                  belowBarData: BarAreaData(
                    show: true,
                    color: Culori.soare.withValues(alpha: 0.12),
                  ),
                ),
                LineChartBarData(
                  spots: puncte(consum),
                  color: Culori.text.withValues(alpha: 0.8),
                  barWidth: 1.5,
                  dotData: const FlDotData(show: false),
                ),
              ],
            ),
            duration: const Duration(milliseconds: 300),
          ),
        ),
        const SizedBox(height: 6),
        Wrap(
          spacing: 14,
          children: [
            _Legenda(Culori.soare, 'producție (soare + vânt)'),
            _Legenda(Culori.text, 'consum'),
          ],
        ),
      ],
    );
  }
}

class _Legenda extends StatelessWidget {
  final Color culoare;
  final String text;
  const _Legenda(this.culoare, this.text);

  @override
  Widget build(BuildContext context) => Row(
    mainAxisSize: MainAxisSize.min,
    children: [
      Container(
        width: 10,
        height: 10,
        decoration: BoxDecoration(
          color: culoare,
          borderRadius: BorderRadius.circular(3),
        ),
      ),
      const SizedBox(width: 6),
      T.mic(text),
    ],
  );
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
                        // cu cat e mai probabil sa fie cineva, cu atat mai verde
                        color: Culori.verde.withValues(
                          alpha:
                              0.08 +
                              (max(
                                        0,
                                        (h < r.p[c].length ? r.p[c][h] : 0) -
                                            10,
                                      ) /
                                      90) *
                                  0.92,
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
                    style: TextStyle(fontSize: 10, color: Culori.text2),
                  ),
                ),
              ),
          ],
        ),
      ],
    );
  }
}
