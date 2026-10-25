// Energie - de unde vine curentul acum, graficul zilei si "ziua de test" (clasic vs smart)
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
    final culoareBaterie = en.modCod == 3
        ? Culori.rosu
        : (en.modCod == 2 ? Culori.soare : Culori.verde);
    final mod = en.modCod.clamp(0, 3);
    final avemTest =
        s.rezultate.length == 2 && s.rezultate[0].valid && s.rezultate[1].valid;

    return Ecran(
      titlu: 'Energie',
      subtitlu: 'la scara machetei, în wați',
      copii: [
        Bloc(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Eticheta(
                'Acum',
                iconita: Icons.bolt_outlined,
                culoare: Culori.soare,
                dreapta: Pastila(
                  'Mod ${en.mod.toLowerCase()}',
                  ton: _tonMod[mod],
                ),
              ),
              IntrinsicHeight(
                child: Row(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    Expanded(child: _Bucata('Soare', '${nr(en.solar, 2)} W')),
                    const SizedBox(width: 10),
                    Expanded(child: _Bucata('Vânt', '${nr(en.vant, 2)} W')),
                  ],
                ),
              ),
              const SizedBox(height: 10),
              IntrinsicHeight(
                child: Row(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    Expanded(
                      child: _Bucata(
                        'Baterie',
                        '${en.baterie}%',
                        jos: Padding(
                          padding: const EdgeInsets.only(top: 8),
                          child: Bara(
                            en.baterie.toDouble(),
                            culoare: culoareBaterie,
                          ),
                        ),
                      ),
                    ),
                    const SizedBox(width: 10),
                    Expanded(child: _Bucata('Consum', '${nr(en.consum, 2)} W')),
                  ],
                ),
              ),
              const SizedBox(height: 12),
              T.mic(_explicatie[mod]),
            ],
          ),
        ),
        Bloc(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Eticheta(
                'Azi',
                iconita: Icons.wb_sunny_outlined,
                culoare: Culori.soare,
              ),
              GraficEnergie(app.istoric),
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
              Eticheta(
                'Ziua de test',
                iconita: Icons.balance_outlined,
                culoare: Culori.verde,
              ),
              const T.mic(
                'Aceeași zi, trăită o dată ca o casă clasică și o dată ca o casă NEPHA. Aceeași familie, aceeași vreme.',
              ),
              const SizedBox(height: 12),
              if (avemTest)
                _Rezultate(s.rezultate[0], s.rezultate[1])
              else
                const T.mic(
                  'Încă n-am rulat ziua de test. Apăsați „Calculează”.',
                ),
              const SizedBox(height: 14),
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

class _Bucata extends StatelessWidget {
  final String eticheta, valoare;
  final Widget? jos;
  const _Bucata(this.eticheta, this.valoare, {this.jos});

  @override
  Widget build(BuildContext context) => Container(
    padding: const EdgeInsets.all(12),
    decoration: BoxDecoration(
      color: Culori.moale,
      borderRadius: BorderRadius.circular(12),
    ),
    child: Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        T.mic(eticheta),
        T(valoare, marime: 22, gros: FontWeight.w700),
        ?jos,
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
          children: [
            T.mare('−${_pct(c.total, s.total)}%', culoare: Culori.verde),
            const SizedBox(width: 12),
            Expanded(
              child: T.mic(
                '${nr(c.total / 1000)} → ${nr(s.total / 1000)} kWh pe zi · ${nr(c.litri, 0)} → ${nr(s.litri, 0)} litri de apă',
              ),
            ),
          ],
        ),
        const SizedBox(height: 8),
        for (final (n, a, b) in randuri)
          Padding(
            padding: const EdgeInsets.only(top: 6),
            child: Row(
              children: [
                Expanded(child: T.mic(n)),
                T('−${_pct(a, b)}%', marime: 13, gros: FontWeight.w700),
                T.mic(' · ${nr(a, 0)} → ${nr(b, 0)} Wh'),
              ],
            ),
          ),
        if (c.mwh > 0) ...[
          const SizedBox(height: 8),
          T.mic(
            'Măsurat pe machetă (INA219): ${nr(c.mwh, 0)} → ${nr(s.mwh, 0)} mWh',
          ),
        ],
      ],
    );
  }
}
