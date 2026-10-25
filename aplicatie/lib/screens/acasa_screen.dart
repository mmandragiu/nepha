// Acasa - pe scurt: cat am economisit azi, cum e in casa si afara, bateria, alarma, ce a decis casa
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../models/api_models.dart';
import '../providers/app_state.dart';
import '../theme/nepha_theme.dart';
import '../widgets/ui.dart';

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

class AcasaScreen extends StatefulWidget {
  final void Function(int fila)
  mergiLa; // ca un card sa poata deschide alta fila
  const AcasaScreen({super.key, required this.mergiLa});

  @override
  State<AcasaScreen> createState() => _AcasaScreenState();
}

class _AcasaScreenState extends State<AcasaScreen> {
  bool _totJurnalul = false;

  @override
  Widget build(BuildContext context) {
    final app = context.watch<AppState>();
    final s = app.stare;
    if (s == null) return _faraDate(app);

    final en = s.energie;
    final al = s.alarma;
    final deschise = al.senzori.where((x) => x.deschis).length;
    final culoareBaterie = en.modCod == 3
        ? Culori.rosu
        : (en.modCod == 2 ? Culori.soare : Culori.verde);
    final decizii = _totJurnalul ? app.jurnal : app.jurnal.take(3).toList();

    return Ecran(
      titlu: salut(s.minut),
      subtitlu:
          'Casa NEPHA · ${s.ora}${s.test == 1 ? ' · ziua de test rulează' : ''}',
      copii: [
        // economia de azi
        Bloc(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Eticheta(
                'Economie azi',
                iconita: Icons.eco_outlined,
                culoare: Culori.verde,
                dreapta: Row(
                  children: [
                    Punct(
                      culoare: s.smart ? Culori.verde : Culori.text2,
                      pulseaza: s.smart,
                    ),
                    const SizedBox(width: 6),
                    T.mic(s.smart ? 'Smart' : 'Clasic'),
                  ],
                ),
              ),
              if (s.smart) ...[
                Row(
                  crossAxisAlignment: CrossAxisAlignment.baseline,
                  textBaseline: TextBaseline.alphabetic,
                  children: [
                    T.mare(nr(en.kwhEconomisiti), culoare: Culori.verde),
                    const T.mediu(' kWh'),
                    const Spacer(),
                    T.mic('≈ ${nr(en.kwhEconomisiti * s.pretKwh)} lei'),
                  ],
                ),
                const T.mic('față de o casă obișnuită, în aceleași condiții'),
                const SizedBox(height: 14),
                _RandCompara('Casă clasică', en.whClasic, 100, Culori.text2),
                const SizedBox(height: 8),
                _RandCompara(
                  'NEPHA',
                  en.whAzi,
                  en.whClasic > 0 ? en.whAzi / en.whClasic * 100 : 0,
                  Culori.verde,
                ),
              ] else
                const T.mic(
                  'Casa e în modul clasic: merge ca o casă obișnuită, fără să economisească.',
                ),
            ],
          ),
        ),

        // in casa / afara
        IntrinsicHeight(
          child: Row(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              Expanded(
                child: Bloc(
                  onTap: () => widget.mergiLa(1),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      const Eticheta(
                        'În casă',
                        iconita: Icons.thermostat_outlined,
                        culoare: Culori.cald,
                      ),
                      T.mare(grade(s.tempCasa)),
                      const SizedBox(height: 8),
                      Pastila(
                        s.incalzire.pornita ? 'încălzește' : 'în repaus',
                        ton: s.incalzire.pornita ? Ton.cald : Ton.gri,
                      ),
                      const SizedBox(height: 6),
                      T.mic(
                        'Țintă ${grade(s.incalzire.tinta)} · ${s.incalzire.motiv}',
                      ),
                    ],
                  ),
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: Bloc(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      const Eticheta(
                        'Afară',
                        iconita: Icons.wb_sunny_outlined,
                        culoare: Culori.apa,
                      ),
                      T.mare(
                        grade(
                          s.afara ?? (s.vreme.valida ? s.vreme.temp : null),
                        ),
                      ),
                      const SizedBox(height: 8),
                      T.mic(textVreme(s.vreme)),
                    ],
                  ),
                ),
              ),
            ],
          ),
        ),

        // bateria
        Bloc(
          onTap: () => widget.mergiLa(2),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Eticheta(
                'Baterie',
                iconita: Icons.bolt_outlined,
                culoare: Culori.soare,
                dreapta: Pastila(
                  'Mod ${en.mod.toLowerCase()}',
                  ton: en.modCod == 3
                      ? Ton.rosu
                      : (en.modCod == 2 ? Ton.soare : Ton.verde),
                ),
              ),
              Row(
                children: [
                  T.mediu('${en.baterie}%'),
                  const SizedBox(width: 14),
                  Expanded(
                    child: Bara(en.baterie.toDouble(), culoare: culoareBaterie),
                  ),
                ],
              ),
            ],
          ),
        ),

        // alarma
        Bloc(
          pericol: al.pericol,
          onTap: () => widget.mergiLa(3),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Eticheta(
                'Alarmă',
                iconita: Icons.shield_outlined,
                culoare: al.pericol
                    ? Culori.rosu
                    : (al.cod == 0 ? Culori.text2 : Culori.verde),
                dreapta: const Icon(
                  Icons.chevron_right,
                  size: 18,
                  color: Culori.text2,
                ),
              ),
              T.mediu(al.stare, culoare: al.pericol ? Culori.rosu : null),
              const SizedBox(height: 4),
              T.mic(
                deschise == 0
                    ? 'Toate ușile și geamurile sunt închise'
                    : '$deschise ${deschise == 1 ? 'ușă sau geam deschis' : 'uși sau geamuri deschise'}',
              ),
            ],
          ),
        ),

        // jurnalul
        Bloc(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Eticheta(
                'Jurnalul casei',
                iconita: Icons.format_list_bulleted,
                dreapta: T.mic('ce a decis și de ce'),
              ),
              if (decizii.isEmpty)
                const T.mic(
                  'Încă nimic. Aici apar deciziile casei, cu motivul lor.',
                ),
              for (var i = 0; i < decizii.length; i++)
                Container(
                  padding: const EdgeInsets.symmetric(vertical: 9),
                  decoration: BoxDecoration(
                    border: i > 0
                        ? const Border(
                            top: BorderSide(color: Culori.linie, width: 0.6),
                          )
                        : null,
                  ),
                  child: Row(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      SizedBox(
                        width: 48,
                        child: T(
                          decizii[i].ora,
                          gros: FontWeight.w700,
                          culoare: Culori.verde,
                        ),
                      ),
                      Expanded(child: T(decizii[i].text)),
                    ],
                  ),
                ),
              if (app.jurnal.length > 3) ...[
                const SizedBox(height: 10),
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
    final text = switch (c) {
      Conexiune.faraAdresa =>
        'Aplicația nu știe încă unde e panoul. Puneți-i adresa la Securitate & Setări (o vedeți pe ecranul panoului), sau porniți modul demo.',
      Conexiune.deconectat =>
        'Panoul nu răspunde. Telefonul trebuie să fie în aceeași rețea Wi-Fi cu casa (sau în rețeaua NEPHA-casa).',
      _ => 'Se conectează la casă...',
    };
    return Ecran(
      titlu: 'NEPHA',
      copii: [
        Bloc(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              T(text),
              if (c == Conexiune.faraAdresa || c == Conexiune.deconectat) ...[
                const SizedBox(height: 14),
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

class _RandCompara extends StatelessWidget {
  final String nume;
  final double wh, procent;
  final Color culoare;
  const _RandCompara(this.nume, this.wh, this.procent, this.culoare);

  @override
  Widget build(BuildContext context) => Row(
    children: [
      SizedBox(width: 94, child: T.mic(nume, culoare: Culori.text)),
      Expanded(
        child: Bara(procent, culoare: culoare, fundal: Culori.moale),
      ),
      SizedBox(
        width: 70,
        child: T.mic(
          '${nr(wh / 1000)} kWh',
          culoare: Culori.text,
          aliniere: TextAlign.right,
        ),
      ),
    ],
  );
}
