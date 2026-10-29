// Acasa - tabloul casei: un mesaj care spune pe scurt ce e important, cat a economisit casa azi,
// cardurile (tinute apasat arata datele tehnice), automatizarile care lucreaza acum si jurnalul
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../models/api_models.dart';
import '../providers/app_state.dart';
import '../theme/nepha_theme.dart';
import '../widgets/automatizari.dart';
import '../widgets/carduri.dart';
import '../widgets/ui.dart';
import '../widgets/ziua_de_test.dart';

String salut(int minut) {
  final h = minut ~/ 60;
  if (h >= 5 && h < 11) return 'Bună dimineața';
  if (h >= 11 && h < 18) return 'Bună ziua';
  return 'Bună seara';
}

String textVreme(Vreme v) {
  if (!v.valida) return 'Fără prognoză (panoul nu are internet)';
  if (v.oraPloaie != null) return 'Ploaie ${v.ploaie}% pe la ${v.oraPloaie}:00';
  return (v.soare ?? 0) > 40
      ? 'Senin în următoarele ore'
      : 'Fără ploaie în 12 ore';
}

// ce trebuie sa stie omul acum, in cateva cuvinte (cele mai grave primele)
List<(String, Ton)> deAtentie(Stare s) => [
  if (s.alarma.cod == 5) ('Alarma sună', Ton.rosu),
  if (s.alarma.cod == 4) ('Cere PIN-ul', Ton.rosu),
  if (!s.arduinoVorbeste) ('Arduino nu răspunde', Ton.cald),
  for (final u in s.alarma.senzori)
    if (u.deschis) ('${u.nume}: deschis', Ton.cald),
  if (s.energie.modCod == 3) ('Baterie aproape goală', Ton.rosu),
  if (s.energie.modCod == 2) ('Baterie slabă', Ton.soare),
  if (s.gradina.umiditate < 25 &&
      !s.gradina.pompa &&
      s.vreme.oraPloaie == null &&
      s.gradina.ploua != true)
    ('Grădina e uscată', Ton.soare),
];

class AcasaScreen extends StatefulWidget {
  final void Function(int fila)
  mergiLa; // ca un card sa poata deschide alta fila
  const AcasaScreen({super.key, required this.mergiLa});

  @override
  State<AcasaScreen> createState() => _AcasaScreenState();
}

class _AcasaScreenState extends State<AcasaScreen> {
  bool _totJurnalul = false;
  bool _toateRegulile = false;

  @override
  Widget build(BuildContext context) {
    final app = context.watch<AppState>();
    final s = app.stare;
    if (s == null) return _faraDate(app);

    final reguli = regulileCasei()
      ..sort((a, b) {
        // cele care lucreaza acum, primele
        bool merge(Regula r) => (!r.doarSmart || s.smart) && r.acum(s);
        return (merge(b) ? 1 : 0) - (merge(a) ? 1 : 0);
      });
    final active = reguli
        .where((r) => (!r.doarSmart || s.smart) && r.acum(s))
        .length;
    final decizii = _totJurnalul ? app.jurnal : app.jurnal.take(3).toList();

    return Ecran(
      subtitlu:
          '${salut(s.minut)} · ${s.ora}${s.test == 1 ? ' · ziua de test rulează' : ''}',
      titlu: 'Casa ta,\n',
      titluTare: 'pe scurt',
      copii: [
        _Rezumat(s),
        _Economie(s, onTap: () => arataZiuaDeTest(context)),
        const Sectiune(
          'Casa acum',
          info: 'Țineți apăsat pe un card pentru date avansate.',
        ),
        GrilaCarduri(carduriCasa(app), mergiLa: widget.mergiLa),
        Sectiune(
          'Automatizări',
          dreapta: Pastila(
            active == 0 ? 'în așteptare' : '$active active acum',
            ton: active == 0 ? Ton.gri : Ton.cald,
          ),
        ),
        Bloc(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Row(
                children: [
                  Expanded(
                    child: T(
                      s.smart ? 'Casa decide singură' : 'Modul clasic',
                      gros: FontWeight.w700,
                    ),
                  ),
                  const SizedBox(width: 10),
                  Comutator(
                    eticheta: 'Modul smart',
                    pornit: app.arata('mod', s.smart),
                    asteapta: app.asteapta('mod'),
                    onSchimba: (v) => app.comanda(
                      v ? 'MOD SMART' : 'MOD CLASIC',
                      cheie: 'mod',
                      valoare: v,
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 14),
              for (final r in _toateRegulile ? reguli : reguli.take(4))
                Padding(
                  padding: const EdgeInsets.only(bottom: 8),
                  child: RandRegula(r, s),
                ),
              const SizedBox(height: 4),
              SizedBox(
                width: double.infinity,
                child: Buton(
                  _toateRegulile
                      ? 'Arată mai puține'
                      : 'Toate regulile (${reguli.length})',
                  onTap: () => setState(() => _toateRegulile = !_toateRegulile),
                ),
              ),
            ],
          ),
        ),
        const Sectiune('Jurnalul casei'),
        Bloc(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              if (decizii.isEmpty) const T.mic('Nicio decizie încă.'),
              for (var i = 0; i < decizii.length; i++)
                _RandJurnal(decizii[i], ultimul: i == decizii.length - 1),
              if (app.jurnal.length > 3) ...[
                const SizedBox(height: 6),
                SizedBox(
                  width: double.infinity,
                  child: Buton(
                    _totJurnalul
                        ? 'Arată mai puțin'
                        : 'Arată tot (${app.jurnal.length})',
                    onTap: () => setState(() => _totJurnalul = !_totJurnalul),
                  ),
                ),
              ],
            ],
          ),
        ),
      ],
    );
  }

  Widget _faraDate(AppState app) {
    final c = app.conexiune;
    final (text, detaliu) = switch (c) {
      Conexiune.faraAdresa => (
        'Niciun panou legat',
        'Adresa e pe ecranul panoului.',
      ),
      Conexiune.deconectat => (
        'Panoul nu răspunde',
        'Telefonul e pe Wi-Fi-ul casei?',
      ),
      _ => ('Se conectează...', null),
    };
    return Ecran(
      titlu: 'NEPHA',
      copii: [
        Bloc(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Iconita(Icons.router_outlined, culoare: c.culoare, marime: 48),
              const SizedBox(height: 14),
              T(text, marime: 18, gros: FontWeight.w700),
              if (detaliu != null) T.mic(detaliu),
              if (c == Conexiune.faraAdresa || c == Conexiune.deconectat) ...[
                const SizedBox(height: 16),
                SizedBox(
                  width: double.infinity,
                  child: Buton(
                    'Deschide setările',
                    principal: true,
                    onTap: () => widget.mergiLa(3),
                  ),
                ),
              ],
            ],
          ),
        ),
      ],
    );
  }
}

// mesajul de sus: siguranta, temperatura si cate lucruri cer atentie
class _Rezumat extends StatelessWidget {
  final Stare s;
  const _Rezumat(this.s);

  @override
  Widget build(BuildContext context) {
    final atentie = deAtentie(s);
    final al = s.alarma;
    final (siguranta, culoare) = switch (al.cod) {
      0 => ('Alarma e oprită.', Culori.text),
      1 => ('Alarma se armează.', Culori.text),
      2 || 3 => ('Sistemul este securizat.', Culori.text),
      4 => ('S-a deschis ușa: introduceți PIN-ul!', Culori.rosu),
      _ => ('ALARMA SUNĂ!', Culori.rosu),
    };
    final stil = fontText(
      marime: 21,
      gros: FontWeight.w400,
      culoare: Culori.text2,
    ).copyWith(height: 1.3, letterSpacing: -0.1);
    final tare = stil.copyWith(fontWeight: FontWeight.w700);

    return Bloc(
      pericol: al.pericol,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text.rich(
            TextSpan(
              style: stil,
              children: [
                TextSpan(
                  text: '$siguranta ',
                  style: tare.copyWith(color: culoare),
                ),
                if (s.tempCasa != null) ...[
                  const TextSpan(text: 'Temperatura medie este de '),
                  TextSpan(
                    text: '${nr(s.tempCasa)}°C',
                    style: tare.copyWith(color: Culori.text),
                  ),
                  const TextSpan(text: '. '),
                ],
                if (atentie.isEmpty)
                  TextSpan(
                    text: 'Totul e în regulă.',
                    style: tare.copyWith(color: Culori.text),
                  )
                else ...[
                  TextSpan(
                    text: atentie.length == 1
                        ? 'Un lucru '
                        : '${atentie.length} lucruri ',
                    style: tare.copyWith(color: Culori.accent),
                  ),
                  const TextSpan(text: 'necesită atenție.'),
                ],
              ],
            ),
          ),
          if (atentie.isNotEmpty) ...[
            const SizedBox(height: 12),
            Wrap(
              spacing: 6,
              runSpacing: 6,
              children: [for (final (t, ton) in atentie) Pastila(t, ton: ton)],
            ),
          ],
        ],
      ),
    );
  }
}

class _RandJurnal extends StatelessWidget {
  final Decizie d;
  final bool ultimul;
  const _RandJurnal(this.d, {required this.ultimul});

  @override
  Widget build(BuildContext context) => IntrinsicHeight(
    child: Row(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        SizedBox(
          width: 46,
          child: T.cod(d.ora, marime: 12, culoare: Culori.accent),
        ),
        // linia timpului: un punct pe fiecare decizie
        SizedBox(
          width: 18,
          child: Column(
            children: [
              const SizedBox(height: 5),
              Container(
                width: 7,
                height: 7,
                decoration: BoxDecoration(
                  color: Culori.accent,
                  shape: BoxShape.circle,
                ),
              ),
              if (!ultimul)
                Expanded(child: Container(width: 1, color: Culori.linie)),
            ],
          ),
        ),
        const SizedBox(width: 8),
        Expanded(
          child: Padding(
            padding: EdgeInsets.only(bottom: ultimul ? 4 : 14),
            child: T(d.text, marime: 14),
          ),
        ),
      ],
    ),
  );
}

// cat a economisit casa azi: cifra mare, apoi doua bare de comparat (casa clasica cu dungi);
// atins, deschide ziua de test
class _Economie extends StatelessWidget {
  final Stare s;
  final VoidCallback onTap;
  const _Economie(this.s, {required this.onTap});

  @override
  Widget build(BuildContext context) {
    final en = s.energie;
    return Bloc(
      onTap: onTap,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              const Expanded(
                child: T('Economie azi', marime: 16, gros: FontWeight.w700),
              ),
              Pastila(
                s.smart ? 'Smart' : 'Clasic',
                ton: s.smart ? Ton.verde : Ton.gri,
                icon: s.smart ? Icons.eco_outlined : null,
              ),
            ],
          ),
          const SizedBox(height: 10),
          if (!s.smart)
            const T.mic('În modul clasic casa nu economisește.')
          else ...[
            Row(
              crossAxisAlignment: CrossAxisAlignment.baseline,
              textBaseline: TextBaseline.alphabetic,
              children: [
                T.mare(nr(en.kwhEconomisiti), marime: 52),
                const SizedBox(width: 6),
                const T('kWh', marime: 18, gros: FontWeight.w600),
                const SizedBox(width: 8),
                Expanded(
                  child: T(
                    '≈ ${nr(en.kwhEconomisiti * s.pretKwh)} lei',
                    marime: 15,
                    gros: FontWeight.w600,
                    culoare: Culori.verde,
                    aliniere: TextAlign.right,
                    randuri: 1,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 16),
            _RandCompara('Casă clasică', en.whClasic, 100, dungi: true),
            const SizedBox(height: 10),
            _RandCompara(
              'NEPHA',
              en.whAzi,
              en.whClasic > 0 ? en.whAzi / en.whClasic * 100 : 0,
            ),
          ],
          // ziua de test: tot cardul o deschide, randul asta doar arata ca se poate
          const SizedBox(height: 14),
          Divider(height: 1, thickness: 0.6, color: Culori.linie),
          const SizedBox(height: 12),
          Row(
            children: [
              const Expanded(child: T.mic('pe consumatori · ziua de test')),
              T(
                'Vezi',
                marime: 13,
                gros: FontWeight.w600,
                culoare: Culori.accent,
              ),
              Icon(Icons.chevron_right_rounded, size: 18, color: Culori.accent),
            ],
          ),
        ],
      ),
    );
  }
}

class _RandCompara extends StatelessWidget {
  final String nume;
  final double wh, procent;
  final bool dungi;
  const _RandCompara(this.nume, this.wh, this.procent, {this.dungi = false});

  @override
  Widget build(BuildContext context) => Row(
    children: [
      SizedBox(width: 92, child: T.mic(nume, culoare: Culori.text)),
      Expanded(
        child: Bara(
          procent,
          inaltime: 12,
          dungi: dungi,
          culoare: dungi ? Culori.text2 : Culori.verde,
        ),
      ),
      SizedBox(
        width: 72,
        child: T.mic(
          '${nr(wh / 1000)} kWh',
          culoare: Culori.text,
          aliniere: TextAlign.right,
        ),
      ),
    ],
  );
}
