// fereastra unei camere (se deschide de jos cand atingi camera): lumina, temperatura, cine e
// acolo si scenele de lumina, ca la un bec inteligent
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../providers/app_state.dart';
import '../theme/nepha_theme.dart';
import 'ui.dart';

IconData iconitaCamera(String nume) {
  final n = nume.toLowerCase();
  if (n.contains('dormitor')) return Icons.bed_outlined;
  if (n.contains('buc')) return Icons.kitchen_outlined;
  if (n.contains('baie')) return Icons.bathtub_outlined;
  return Icons.weekend_outlined;
}

void arataCamera(BuildContext context, int i) => showModalBottomSheet<void>(
  context: context,
  isScrollControlled: true,
  builder: (_) => _FereastraCamera(i),
);

// cat de tare e becul in fiecare scena (LUMINA <camera> <procent>)
const _scene = [
  ('Noapte', Icons.nightlight_outlined, 15),
  ('Relax', Icons.spa_outlined, 45),
  ('Citit', Icons.menu_book_outlined, 75),
  ('Plin', Icons.wb_sunny_outlined, 100),
];

class _FereastraCamera extends StatelessWidget {
  final int i;
  const _FereastraCamera(this.i);

  @override
  Widget build(BuildContext context) {
    final app = context.watch<AppState>();
    final s = app.stare;
    if (s == null || i >= s.camere.length) return const SizedBox(height: 120);
    final c = s.camere[i];
    final cheie = 'lumina:$i';
    final nivel = app.arata(cheie, c.lumina);
    return SafeArea(
      child: Padding(
        padding: const EdgeInsets.fromLTRB(20, 0, 20, 20),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Iconita(iconitaCamera(c.nume), aprinsa: nivel > 0, marime: 48),
                const SizedBox(width: 14),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      T(c.nume, marime: 22, gros: FontWeight.w700),
                      T.mic(
                        '${c.prezenta ? 'e cineva acum' : 'nu e nimeni'}${c.fereastra ? ' · geam deschis' : ''}',
                      ),
                    ],
                  ),
                ),
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
            const SizedBox(height: 20),
            Row(
              children: [
                Expanded(
                  child: _Valoare(
                    'Temperatură',
                    grade(c.temp),
                    Icons.thermostat_outlined,
                  ),
                ),
                const SizedBox(width: 10),
                Expanded(
                  child: _Valoare('Lumină', '$nivel%', Icons.lightbulb_outline),
                ),
                const SizedBox(width: 10),
                Expanded(
                  child: _Valoare(
                    'Mișcare',
                    c.prezenta ? 'da' : 'nu',
                    Icons.sensors,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 20),
            const T('Scene', gros: FontWeight.w700),
            const SizedBox(height: 10),
            // ca in poza: o capsula cu scenele, cea aleasa e un cerc portocaliu
            Container(
              padding: const EdgeInsets.all(6),
              decoration: BoxDecoration(
                color: Culori.moale,
                borderRadius: BorderRadius.circular(coltCard),
              ),
              child: Row(
                children: [
                  for (final (nume, icon, p) in _scene)
                    Expanded(
                      child: _Scena(
                        nume,
                        icon,
                        aleasa: nivel == p,
                        onTap: () => app.comanda(
                          'LUMINA $i $p',
                          cheie: cheie,
                          valoare: p,
                        ),
                      ),
                    ),
                ],
              ),
            ),
            const SizedBox(height: 14),
            SizedBox(
              width: double.infinity,
              child: Buton(
                'Lasă casa să decidă',
                icon: Icons.auto_awesome_outlined,
                onTap: () => app.comanda('LUMINA $i AUTO'),
              ),
            ),
            const SizedBox(height: 8),
            const T.mic(
              'Aprinsă sau stinsă de aici, lumina rămâne așa o oră. Apoi o preia din nou casa.',
            ),
          ],
        ),
      ),
    );
  }
}

class _Valoare extends StatelessWidget {
  final String eticheta, valoare;
  final IconData icon;
  const _Valoare(this.eticheta, this.valoare, this.icon);

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
        Icon(icon, size: 18, color: Culori.text2),
        const SizedBox(height: 10),
        T(valoare, marime: 22, gros: FontWeight.w300),
        T.mic(eticheta),
      ],
    ),
  );
}

class _Scena extends StatelessWidget {
  final String nume;
  final IconData icon;
  final bool aleasa;
  final VoidCallback onTap;
  const _Scena(
    this.nume,
    this.icon, {
    required this.aleasa,
    required this.onTap,
  });

  @override
  Widget build(BuildContext context) => GestureDetector(
    behavior: HitTestBehavior.opaque,
    onTap: () {
      atingere();
      onTap();
    },
    child: Padding(
      padding: const EdgeInsets.symmetric(vertical: 6),
      child: Column(
        children: [
          AnimatedContainer(
            duration: const Duration(milliseconds: 220),
            width: 42,
            height: 42,
            decoration: BoxDecoration(
              color: aleasa ? Culori.accent : Culori.card,
              shape: BoxShape.circle,
            ),
            child: Icon(
              icon,
              size: 20,
              color: aleasa ? Colors.white : Culori.text,
            ),
          ),
          const SizedBox(height: 6),
          T(
            nume,
            marime: 12,
            gros: FontWeight.w600,
            culoare: aleasa ? Culori.text : Culori.text2,
          ),
        ],
      ),
    ),
  );
}
