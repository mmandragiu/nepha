// ziua de test - aceeasi zi simulata o data ca o casa clasica si o data ca NEPHA.
// Se deschide de jos din cardul "Economie azi" de pe Acasa.
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../models/api_models.dart';
import '../providers/app_state.dart';
import '../theme/nepha_theme.dart';
import 'ui.dart';

void arataZiuaDeTest(BuildContext context) => showModalBottomSheet<void>(
  context: context,
  isScrollControlled: true,
  builder: (_) => const _ZiuaDeTest(),
);

class _ZiuaDeTest extends StatelessWidget {
  const _ZiuaDeTest();

  @override
  Widget build(BuildContext context) {
    final app = context.watch<AppState>();
    final s = app.stare;
    if (s == null) return const SizedBox(height: 120);
    final avem =
        s.rezultate.length == 2 && s.rezultate[0].valid && s.rezultate[1].valid;
    return SafeArea(
      child: SingleChildScrollView(
        padding: const EdgeInsets.fromLTRB(20, 0, 20, 20),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Row(
              children: [
                T('Clasic vs NEPHA', marime: 22, gros: FontWeight.w700),
                Info(
                  'Aceeași zi, cu aceeași familie și aceeași vreme, trăită o dată ca o casă obișnuită și o dată ca NEPHA.',
                ),
              ],
            ),
            const T.mic('ziua de test'),
            const SizedBox(height: 18),
            if (avem)
              _Rezultate(s.rezultate[0], s.rezultate[1])
            else
              const T.mic('Încă n-a rulat.'),
            const SizedBox(height: 18),
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
                    s.test == 1 ? 'Oprește' : 'Pe machetă',
                    onTap: () =>
                        app.comanda(s.test == 1 ? 'TEST STOP' : 'TEST SMART'),
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
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
          T.mic('Măsurat pe machetă: ${nr(c.mwh, 0)} → ${nr(s.mwh, 0)} mWh'),
        ],
      ],
    );
  }
}
