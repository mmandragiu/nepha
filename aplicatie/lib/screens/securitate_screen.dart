// Securitate & Setari - alarma (PIN, Face ID / Touch ID sau amprenta), usile / geamurile,
// unde e panoul, cum se poarta casa
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../providers/app_state.dart';
import '../theme/nepha_theme.dart';
import '../widgets/tastatura_pin.dart';
import '../widgets/ui.dart';

const _explicatieAlarma = [
  'Când plecați, armați-o: casa știe atunci sigur că e goală și economisește.',
  'Aveți 30 de secunde să ieșiți și să închideți ușa.',
  'Suntem acasă și dormim: păzim doar ușile și geamurile.',
  'Casa e păzită și economisește: lumini stinse, încălzire redusă, priza TV oprită.',
  'S-a deschis ușa. Introduceți PIN-ul!',
  'Alarma sună! Introduceți PIN-ul ca s-o opriți.',
];

class SecuritateScreen extends StatefulWidget {
  const SecuritateScreen({super.key});

  @override
  State<SecuritateScreen> createState() => _SecuritateScreenState();
}

class _SecuritateScreenState extends State<SecuritateScreen> {
  late final _adresa = TextEditingController(
    text: context.read<AppState>().adresa,
  );

  @override
  void dispose() {
    _adresa.dispose();
    super.dispose();
  }

  Future<void> _dezarmeaza(AppState app) async {
    // intai Face ID / Touch ID / amprenta (daca am pastrat PIN-ul), altfel tastatura
    if (app.amprentaPornita &&
        app.areSalvatPin &&
        await app.dezarmeazaCuAmprenta()) {
      return;
    }
    if (!mounted) return;
    final pin = await ceAPin(context);
    if (pin != null) await app.dezarmeazaCuPin(pin);
  }

  void _conecteaza(AppState app) {
    final a = _adresa.text.trim();
    if (a.isEmpty) {
      app.anunta(
        'Scrieți adresa panoului (o vedeți pe ecranul lui, la Setări)',
        eroare: true,
      );
      return;
    }
    FocusScope.of(context).unfocus();
    app.schimbaSetari(adresa: a, demo: false);
    app.anunta('Mă conectez la $a');
  }

  @override
  Widget build(BuildContext context) {
    final app = context.watch<AppState>();
    final s = app.stare;

    return Ecran(
      titlu: 'Securitate & Setări',
      copii: [
        // ---- alarma
        if (s != null)
          Bloc(
            pericol: s.alarma.pericol,
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  children: [
                    Icon(
                      Icons.shield_outlined,
                      size: 30,
                      color: s.alarma.pericol
                          ? Culori.rosu
                          : (s.alarma.cod == 0 ? Culori.text2 : Culori.verde),
                    ),
                    const SizedBox(width: 12),
                    Expanded(
                      child: T.mediu(
                        s.alarma.stare,
                        culoare: s.alarma.pericol ? Culori.rosu : null,
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 8),
                T.mic(_explicatieAlarma[s.alarma.cod.clamp(0, 5)]),
                const SizedBox(height: 16),
                if (s.alarma.cod == 0)
                  Row(
                    children: [
                      Expanded(
                        child: Buton(
                          'Plec de acasă',
                          principal: true,
                          onTap: () => app.comanda('ALARMA PLECAT'),
                        ),
                      ),
                      const SizedBox(width: 8),
                      Expanded(
                        child: Buton(
                          'Noapte',
                          onTap: () => app.comanda('ALARMA ACASA'),
                        ),
                      ),
                    ],
                  )
                else
                  SizedBox(
                    width: double.infinity,
                    child: Buton(
                      app.amprentaPornita && app.areSalvatPin
                          ? 'Dezarmează (${app.numeBiometrie} sau PIN)'
                          : 'Dezarmează',
                      principal: true,
                      onTap: () => _dezarmeaza(app),
                    ),
                  ),
              ],
            ),
          ),

        // ---- usi si geamuri
        if (s != null)
          Bloc(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Eticheta(
                  'Uși și geamuri',
                  iconita: Icons.sensor_door_outlined,
                ),
                for (var i = 0; i < s.alarma.senzori.length; i++)
                  Builder(
                    builder: (_) {
                      final cheie = 'usa:$i';
                      final deschis = app.arata(
                        cheie,
                        s.alarma.senzori[i].deschis,
                      );
                      return Container(
                        padding: const EdgeInsets.symmetric(vertical: 10),
                        decoration: BoxDecoration(
                          border: i > 0
                              ? const Border(
                                  top: BorderSide(
                                    color: Culori.linie,
                                    width: 0.6,
                                  ),
                                )
                              : null,
                        ),
                        child: Row(
                          children: [
                            Expanded(child: T(s.alarma.senzori[i].nume)),
                            Pastila(
                              deschis ? 'deschis' : 'închis',
                              ton: deschis ? Ton.cald : Ton.gri,
                              asteapta: app.asteapta(cheie),
                              onTap: () => app.comanda(
                                'USA $i ${deschis ? 0 : 1}',
                                cheie: cheie,
                                valoare: !deschis,
                              ),
                            ),
                          ],
                        ),
                      );
                    },
                  ),
                const SizedBox(height: 6),
                const T.mic(
                  'Dacă o stare nu e cea reală (telecomanda de pe ușă s-a oprit singură), atingeți-o ca s-o schimbați. Rămâne așa până transmite senzorul din nou și nu pornește alarma.',
                ),
              ],
            ),
          ),

        // ---- panoul casei
        Bloc(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Eticheta('Panoul casei', iconita: Icons.router_outlined),
              const T.mic(
                'Adresa e scrisă pe ecranul panoului, la Setări (de exemplu 192.168.1.42). Telefonul trebuie să fie în aceeași rețea Wi-Fi.',
              ),
              const SizedBox(height: 12),
              TextField(
                controller: _adresa,
                keyboardType: TextInputType.url,
                autocorrect: false,
                onSubmitted: (_) => _conecteaza(app),
                decoration: InputDecoration(
                  hintText: '192.168.1.42',
                  isDense: true,
                  border: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(12),
                    borderSide: const BorderSide(color: Culori.linie),
                  ),
                  enabledBorder: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(12),
                    borderSide: const BorderSide(color: Culori.linie),
                  ),
                ),
              ),
              const SizedBox(height: 12),
              SizedBox(
                width: double.infinity,
                child: Buton(
                  'Conectează',
                  principal: true,
                  onTap: () => _conecteaza(app),
                ),
              ),
              const SizedBox(height: 14),
              _RandSetare(
                titlu: 'Mod demo',
                detaliu: 'o seară simulată, fără panou (pentru prezentări)',
                dreapta: Comutator(
                  eticheta: 'Mod demo',
                  pornit: app.demo,
                  culoare: Culori.verde,
                  onSchimba: (d) => app.schimbaSetari(demo: d),
                ),
              ),
              if (s != null && s.retea.ip.isNotEmpty) ...[
                const SizedBox(height: 10),
                T.mic(
                  'Panoul e în ${s.retea.ap ? 'rețeaua proprie NEPHA-casa' : (s.retea.nume.isEmpty ? 'Wi-Fi' : s.retea.nume)} · ${s.retea.ip}',
                ),
              ],
            ],
          ),
        ),

        // ---- cum se poarta casa
        Bloc(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Eticheta(
                'Cum se poartă casa',
                iconita: Icons.eco_outlined,
                culoare: Culori.verde,
              ),
              _ModCasa(app),
              const SizedBox(height: 16),
              _RandSetare(
                titlu: 'Temperatura de confort',
                detaliu: 'cât de cald să fie când e cineva acasă',
                dreapta: Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    _ButonMic('−', () => _confort(app, -0.5)),
                    SizedBox(
                      width: 52,
                      child: T(
                        '${nr(app.arata('confort', app.confort))}°',
                        marime: 17,
                        gros: FontWeight.w700,
                        aliniere: TextAlign.center,
                      ),
                    ),
                    _ButonMic('+', () => _confort(app, 0.5)),
                  ],
                ),
              ),
              if (app.amprentaDisponibila) ...[
                const SizedBox(height: 14),
                _RandSetare(
                  titlu: 'Dezarmare cu ${app.numeBiometrie}',
                  detaliu: app.areSalvatPin
                      ? 'PIN-ul e păstrat criptat pe telefon'
                      : 'după prima dezarmare cu PIN, data viitoare ajunge ${app.numeBiometrie}',
                  dreapta: Comutator(
                    eticheta: 'Dezarmare cu ${app.numeBiometrie}',
                    pornit: app.amprentaPornita,
                    culoare: Culori.verde,
                    onSchimba: (v) => app.schimbaSetari(amprenta: v),
                  ),
                ),
              ],
            ],
          ),
        ),
        const T.mic(
          'NEPHA · casa care se gândește singură cum să economisească energie',
          aliniere: TextAlign.center,
        ),
      ],
    );
  }

  Future<void> _confort(AppState app, double pas) async {
    final nou = ((app.arata('confort', app.confort) + pas) * 2).round() / 2;
    final t = nou.clamp(16, 26).toDouble();
    // o tinem minte doar daca a ajuns la casa
    if (await app.comanda(
      'TEMP CONFORT ${(t * 10).round()}',
      cheie: 'confort',
      valoare: t,
    )) {
      await app.schimbaSetari(confort: t);
    }
  }
}

class _ModCasa extends StatelessWidget {
  final AppState app;
  const _ModCasa(this.app);

  @override
  Widget build(BuildContext context) {
    final s = app.stare;
    final smart = s == null
        ? null
        : app.arata('mod', s.smart); // fara date nu stim modul
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Container(
          padding: const EdgeInsets.all(4),
          decoration: BoxDecoration(
            color: Culori.moale,
            borderRadius: BorderRadius.circular(99),
          ),
          child: Row(
            children: [
              for (final m in [true, false])
                Expanded(
                  child: GestureDetector(
                    onTap: () {
                      if (smart == null || smart == m) return;
                      atingere();
                      app.comanda(
                        m ? 'MOD SMART' : 'MOD CLASIC',
                        cheie: 'mod',
                        valoare: m,
                      );
                    },
                    child: AnimatedContainer(
                      duration: const Duration(milliseconds: 200),
                      padding: const EdgeInsets.symmetric(vertical: 8),
                      decoration: BoxDecoration(
                        color: smart == m ? Culori.card : Colors.transparent,
                        borderRadius: BorderRadius.circular(99),
                        boxShadow: smart == m ? umbraCard : null,
                      ),
                      child: T(
                        '${m ? 'Smart' : 'Clasic'}${smart == m && app.asteapta('mod') ? '…' : ''}',
                        gros: FontWeight.w700,
                        culoare: smart == m ? Culori.text : Culori.text2,
                        aliniere: TextAlign.center,
                      ),
                    ),
                  ),
                ),
            ],
          ),
        ),
        const SizedBox(height: 8),
        T.mic(
          smart == null
              ? 'Modul apare când aplicația vorbește cu casa.'
              : smart
              ? 'Casa ia singură deciziile care economisesc energie.'
              : 'Casa merge ca una obișnuită (pentru comparație).',
        ),
      ],
    );
  }
}

class _RandSetare extends StatelessWidget {
  final String titlu, detaliu;
  final Widget dreapta;
  const _RandSetare({
    required this.titlu,
    required this.detaliu,
    required this.dreapta,
  });

  @override
  Widget build(BuildContext context) => Row(
    children: [
      Expanded(
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [T(titlu), T.mic(detaliu)],
        ),
      ),
      const SizedBox(width: 10),
      dreapta,
    ],
  );
}

class _ButonMic extends StatelessWidget {
  final String text;
  final VoidCallback onTap;
  const _ButonMic(this.text, this.onTap);

  @override
  Widget build(BuildContext context) => SizedBox(
    width: 40,
    height: 36,
    child: OutlinedButton(
      onPressed: () {
        atingere();
        onTap();
      },
      style: OutlinedButton.styleFrom(
        padding: EdgeInsets.zero,
        side: const BorderSide(color: Culori.linie),
        shape: const StadiumBorder(),
        foregroundColor: Culori.text,
      ),
      child: Text(
        text,
        style: const TextStyle(fontSize: 18, fontWeight: FontWeight.w700),
      ),
    ),
  );
}
