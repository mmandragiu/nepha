// automatizarile casei, desenate ca doua noduri legate: [DACA] → [ATUNCI].
// Regulile sunt scrise in programul lui Arduino (aplicatia nu le schimba); aici aratam care
// lucreaza chiar acum: nodurile ei capata culoarea accentului.
import 'package:flutter/material.dart';

import '../models/api_models.dart';
import '../theme/nepha_theme.dart';
import 'ui.dart';

class Regula {
  final IconData dacaIcon, atunciIcon;
  final String daca, atunci;
  final bool doarSmart; // in modul clasic casa n-o foloseste
  final bool Function(Stare s) acum; // se intampla chiar acum?
  const Regula({
    required this.dacaIcon,
    required this.daca,
    required this.atunciIcon,
    required this.atunci,
    this.doarSmart = true,
    required this.acum,
  });
}

// miscare = casa are senzori de miscare; fara ei luminile le aprindeti voi, iar casa le
// stinge cand plecati
List<Regula> regulileCasei({required bool miscare}) => [
  if (miscare) ...[
    Regula(
      dacaIcon: Icons.directions_walk,
      daca: 'Mișcare în cameră',
      atunciIcon: Icons.lightbulb_outline,
      atunci: 'Aprinde lumina',
      acum: (s) => s.camere.any((c) => c.prezenta && c.lumina > 0),
    ),
    Regula(
      dacaIcon: Icons.person_off_outlined,
      daca: 'Camera rămâne goală',
      atunciIcon: Icons.lightbulb_outline,
      atunci: 'Stinge lumina',
      acum: (s) => s.camere.any((c) => !c.prezenta && c.lumina == 0),
    ),
  ] else
    Regula(
      dacaIcon: Icons.logout,
      daca: 'Plecați de acasă',
      atunciIcon: Icons.lightbulb_outline,
      atunci: 'Stinge toate luminile',
      acum: (s) => s.alarma.cod == 3,
    ),
  Regula(
    dacaIcon: Icons.window_outlined,
    daca: 'Geam deschis',
    atunciIcon: Icons.local_fire_department_outlined,
    atunci: 'Oprește încălzirea',
    acum: (s) => s.incalzire.motivCod == 6,
  ),
  Regula(
    dacaIcon: Icons.schedule,
    daca: 'Vine cineva curând',
    atunciIcon: Icons.local_fire_department_outlined,
    atunci: 'Încălzește dinainte',
    acum: (s) => s.incalzire.motivCod == 2,
  ),
  Regula(
    dacaIcon: Icons.logout,
    daca: 'Nu e nimeni acasă',
    atunciIcon: Icons.eco_outlined,
    atunci: 'Încălzire redusă',
    acum: (s) => s.incalzire.motivCod == 3 || s.alarma.cod == 3,
  ),
  Regula(
    dacaIcon: Icons.umbrella_outlined,
    daca: 'Vine ploaia',
    atunciIcon: Icons.water_drop_outlined,
    atunci: 'Nu udă grădina',
    acum: (s) =>
        (s.vreme.oraPloaie != null || s.gradina.ploua == true) &&
        !s.gradina.pompa,
  ),
  Regula(
    dacaIcon: Icons.grass_outlined,
    daca: 'Pământ uscat',
    atunciIcon: Icons.water_drop_outlined,
    atunci: 'Pornește pompa',
    acum: (s) => s.gradina.pompa,
  ),
  Regula(
    dacaIcon: Icons.tv_outlined,
    daca: 'TV în standby',
    atunciIcon: Icons.power_off_outlined,
    atunci: 'Taie priza',
    acum: (s) => !s.priza,
  ),
  Regula(
    dacaIcon: Icons.battery_alert_outlined,
    daca: 'Baterie slabă',
    atunciIcon: Icons.eco_outlined,
    atunci: 'Economie de energie',
    acum: (s) => s.energie.modCod >= 2,
  ),
  Regula(
    dacaIcon: Icons.wb_sunny_outlined,
    daca: 'Baterie plină și soare',
    atunciIcon: Icons.bolt_outlined,
    atunci: 'Pornește ce poate aștepta',
    acum: (s) => s.energie.modCod == 1,
  ),
  Regula(
    dacaIcon: Icons.sensor_door_outlined,
    daca: 'Ușă deschisă, alarmă armată',
    atunciIcon: Icons.campaign_outlined,
    atunci: 'Cere PIN, apoi sirena',
    doarSmart: false,
    acum: (s) => s.alarma.cod >= 4,
  ),
];

class RandRegula extends StatelessWidget {
  final Regula r;
  final Stare s;
  const RandRegula(this.r, this.s, {super.key});

  @override
  Widget build(BuildContext context) {
    final oprita = r.doarSmart && !s.smart;
    final acum = !oprita && r.acum(s);
    return AnimatedOpacity(
      duration: const Duration(milliseconds: 300),
      opacity: oprita ? 0.45 : 1,
      child: Row(
        children: [
          Expanded(child: _Nod('Dacă', r.dacaIcon, r.daca, acum)),
          SizedBox(
            width: 28,
            child: Icon(
              Icons.arrow_forward_rounded,
              size: 18,
              color: acum ? Culori.accent : Culori.text2,
            ),
          ),
          Expanded(child: _Nod('Atunci', r.atunciIcon, r.atunci, acum)),
        ],
      ),
    );
  }
}

class _Nod extends StatelessWidget {
  final String tip, text;
  final IconData icon;
  final bool aprins;
  const _Nod(this.tip, this.icon, this.text, this.aprins);

  @override
  Widget build(BuildContext context) => AnimatedContainer(
    duration: const Duration(milliseconds: 300),
    padding: const EdgeInsets.fromLTRB(8, 8, 10, 8),
    decoration: BoxDecoration(
      color: aprins ? Culori.accentD : Culori.moale,
      borderRadius: BorderRadius.circular(coltMic),
    ),
    child: Row(
      children: [
        AnimatedContainer(
          duration: const Duration(milliseconds: 300),
          width: 30,
          height: 30,
          decoration: BoxDecoration(
            color: aprins ? Culori.accent : Culori.card,
            shape: BoxShape.circle,
          ),
          child: Icon(
            icon,
            size: 16,
            color: aprins ? Colors.white : Culori.text,
          ),
        ),
        const SizedBox(width: 8),
        Expanded(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              T(tip, marime: 11, culoare: Culori.text2),
              T(text, marime: 13, gros: FontWeight.w700, randuri: 2),
            ],
          ),
        ),
      ],
    ),
  );
}
