// Camere & Gradina - cine e unde, luminile (se aprind de aici), gradina si ce a invatat casa
import 'dart:math';

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../providers/app_state.dart';
import '../theme/nepha_theme.dart';
import '../widgets/grafice.dart';
import '../widgets/ui.dart';

class CamereScreen extends StatelessWidget {
  const CamereScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final app = context.watch<AppState>();
    final s = app.stare;
    final r = app.rutina;
    final zile = r == null || r.zile.isEmpty ? 0 : r.zile.reduce(max);

    return Ecran(
      titlu: 'Camere & Grădină',
      subtitlu: 'atingeți comutatorul ca să aprindeți becul',
      copii: [
        if (s == null) const T.mic('Se așteaptă datele de la casă...'),
        if (s != null)
          for (var i = 0; i < s.camere.length; i++)
            Builder(
              builder: (_) {
                final c = s.camere[i];
                final cheie = 'lumina:$i';
                return Bloc(
                  child: Row(
                    children: [
                      Expanded(
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            T(c.nume, marime: 17, gros: FontWeight.w700),
                            const SizedBox(height: 2),
                            T.mic(
                              '${c.prezenta ? 'e cineva' : 'liber'}${c.lumina > 0 ? ' · lumină ${c.lumina}%' : ''}',
                            ),
                            if (c.fereastra) ...[
                              const SizedBox(height: 6),
                              const Pastila('geam deschis', ton: Ton.cald),
                            ],
                          ],
                        ),
                      ),
                      T.mediu(grade(c.temp)),
                      const SizedBox(width: 14),
                      Comutator(
                        eticheta: 'Lumina din ${c.nume}',
                        pornit: app.arata(cheie, c.lumina > 0),
                        asteapta: app.asteapta(cheie),
                        onSchimba: (nou) => app.comanda(
                          'LUMINA $i ${nou ? 100 : 0}',
                          cheie: cheie,
                          valoare: nou,
                        ),
                      ),
                    ],
                  ),
                );
              },
            ),

        // gradina
        if (s != null)
          Bloc(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Eticheta(
                  'Grădină',
                  iconita: Icons.water_drop_outlined,
                  culoare: Culori.apa,
                ),
                Row(
                  children: [
                    Inel(s.gradina.umiditate),
                    const SizedBox(width: 16),
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.stretch,
                        children: [
                          T.mic(
                            s.gradina.pompa
                                ? 'Se udă acum.'
                                : s.gradina.umiditate >= 35
                                ? 'Pământul e destul de umed.'
                                : 'Pământul e uscat: casa așteaptă momentul potrivit (nu udă dacă vine ploaia).',
                          ),
                          const SizedBox(height: 10),
                          Buton(
                            'Udă acum',
                            onTap: s.gradina.pompa
                                ? null
                                : () => app.comanda('UDA'),
                          ),
                        ],
                      ),
                    ),
                  ],
                ),
              ],
            ),
          ),

        // ce a invatat casa
        Bloc(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Eticheta(
                'Ce a învățat casa',
                iconita: Icons.grid_view_outlined,
                culoare: Culori.verde,
                dreapta: r != null && r.valida
                    ? T.mic('din $zile ${zile == 1 ? 'zi' : 'zile'}')
                    : null,
              ),
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
