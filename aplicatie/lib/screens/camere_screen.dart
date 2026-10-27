// Camere & Gradina - fiecare camera ca un card (lumina se aprinde de aici, atingerea deschide
// camera), priza TV, gradina si ce a invatat casa
import 'dart:math';

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../models/api_models.dart';
import '../providers/app_state.dart';
import '../widgets/grafice.dart';
import '../widgets/fereastra_camera.dart';
import '../widgets/ui.dart';

class CamereScreen extends StatelessWidget {
  const CamereScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final app = context.watch<AppState>();
    final s = app.stare;
    final r = app.rutina;
    final zile = r == null || r.zile.isEmpty ? 0 : r.zile.reduce(max);

    final carduri = <Widget>[
      if (s != null) ...[
        for (var i = 0; i < s.camere.length; i++) _CardCamera(i, s.camere[i]),
        _CardPriza(s),
      ],
    ];

    return Ecran(
      titlu: 'Camere',
      subtitlu: 'atingeți o cameră pentru lumină și detalii',
      copii: [
        if (s == null) const T.mic('Se așteaptă datele de la casă...'),
        // camerele, cate doua pe rand
        for (var k = 0; k < carduri.length; k += 2)
          IntrinsicHeight(
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                Expanded(child: carduri[k]),
                const SizedBox(width: 12),
                Expanded(
                  child: k + 1 < carduri.length
                      ? carduri[k + 1]
                      : const SizedBox(),
                ),
              ],
            ),
          ),

        // gradina
        if (s != null) ...[
          const Sectiune('Grădină'),
          Bloc(
            child: Row(
              children: [
                Inel(s.gradina.umiditate, marime: 116),
                const SizedBox(width: 18),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.stretch,
                    children: [
                      T(
                        s.gradina.pompa
                            ? 'Se udă acum'
                            : s.gradina.umiditate >= 35
                            ? 'Pământul e umed'
                            : 'Pământul e uscat',
                        marime: 17,
                        gros: FontWeight.w700,
                      ),
                      const SizedBox(height: 2),
                      T.mic(
                        s.gradina.pompa
                            ? 'Pompa merge câteva secunde.'
                            : s.vreme.oraPloaie != null
                            ? 'Vine ploaia pe la ${s.vreme.oraPloaie}:00: casa nu udă degeaba.'
                            : 'Casa udă singură la momentul potrivit.',
                      ),
                      const SizedBox(height: 12),
                      Buton(
                        'Udă acum',
                        icon: Icons.water_drop_outlined,
                        onTap: s.gradina.pompa
                            ? null
                            : () => app.comanda('UDA'),
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),
        ],

        // ce a invatat casa
        Sectiune(
          'Ce a învățat casa',
          dreapta: r != null && r.valida
              ? T.mic('din $zile ${zile == 1 ? 'zi' : 'zile'}')
              : null,
        ),
        Bloc(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              HartaRutina(r, oraAcum: s == null ? -1 : s.minut ~/ 60),
              const SizedBox(height: 12),
              const T.mic(
                'Cât de probabil e să fie cineva în fiecare cameră, la fiecare oră. Din asta casa știe când să pornească încălzirea mai devreme și când să stingă lumina mai repede.',
              ),
            ],
          ),
        ),
      ],
    );
  }
}

class _CardCamera extends StatelessWidget {
  final int i;
  final Camera c;
  const _CardCamera(this.i, this.c);

  @override
  Widget build(BuildContext context) {
    final app = context.watch<AppState>();
    final cheie = 'lumina:$i';
    final nivel = app.arata(cheie, c.lumina);
    return Bloc(
      padding: const EdgeInsets.all(16),
      onTap: () => arataCamera(context, i),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Iconita(iconitaCamera(c.nume), aprinsa: nivel > 0),
              const Spacer(),
              Comutator(
                eticheta: 'Lumina din ${c.nume}',
                pornit: nivel > 0,
                asteapta: app.asteapta(cheie),
                onSchimba: (nou) => app.comanda(
                  'LUMINA $i ${nou ? 100 : 0}',
                  cheie: cheie,
                  valoare: nou ? 100 : 0,
                ),
              ),
            ],
          ),
          const SizedBox(height: 14),
          T(c.nume, marime: 17, gros: FontWeight.w700, randuri: 1),
          T.mic(nivel > 0 ? 'lumină $nivel%' : 'stinsă'),
          const Spacer(),
          const SizedBox(height: 12),
          Wrap(
            spacing: 6,
            runSpacing: 6,
            children: [
              Pastila(grade(c.temp), icon: Icons.thermostat_outlined),
              if (c.prezenta) const Pastila('e cineva', ton: Ton.cald),
              if (c.fereastra) const Pastila('geam deschis', ton: Ton.cald),
            ],
          ),
        ],
      ),
    );
  }
}

class _CardPriza extends StatelessWidget {
  final Stare s;
  const _CardPriza(this.s);

  @override
  Widget build(BuildContext context) {
    final app = context.watch<AppState>();
    final pornita = app.arata('priza', s.priza);
    return Bloc(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Iconita(Icons.tv_outlined, aprinsa: pornita),
              const Spacer(),
              Comutator(
                eticheta: 'Priza televizorului',
                pornit: pornita,
                asteapta: app.asteapta('priza'),
                onSchimba: (nou) => app.comanda(
                  'PRIZA ${nou ? 1 : 0}',
                  cheie: 'priza',
                  valoare: nou,
                ),
              ),
            ],
          ),
          const SizedBox(height: 14),
          const T('Priza TV', marime: 17, gros: FontWeight.w700),
          T.mic(pornita ? 'pornită' : 'oprită'),
          const Spacer(),
          const SizedBox(height: 12),
          // inapoi pe automat: casa o taie singura cand televizorul sta in standby
          Pastila(
            'automat',
            ton: Ton.verde,
            onTap: () => app.comanda('PRIZA AUTO'),
          ),
        ],
      ),
    );
  }
}
