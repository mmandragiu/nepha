// Securitate & Setari - alarma (PIN, Face ID / Touch ID sau amprenta), usile / geamurile,
// unde e panoul, cum se poarta casa, tema de zi / noapte
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';

import '../providers/app_state.dart';
import '../theme/nepha_theme.dart';
import '../widgets/tastatura_pin.dart';
import '../widgets/ui.dart';

const _explicatieAlarma = [
  '',
  'Aveți 30 de secunde să ieșiți.',
  'Doar ușile și geamurile.',
  'Casa economisește cât e goală.',
  'Introduceți PIN-ul.',
  'Introduceți PIN-ul ca s-o opriți.',
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
    final pin = await ceAPin(context, demo: app.demo);
    if (pin != null) await app.dezarmeazaCuPin(pin);
  }

  bool _intreb = false;

  // dupa prima dezarmare reusita cu PIN: "Data viitoare cu Face ID?"
  Future<void> _propuneBiometria(AppState app) async {
    final da = await showDialog<bool>(
      context: context,
      builder: (c) => AlertDialog(
        backgroundColor: Culori.card,
        surfaceTintColor: Colors.transparent,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(coltCard),
        ),
        icon: Icon(
          app.numeBiometrie == 'Face ID'
              ? Icons.face_retouching_natural
              : Icons.fingerprint,
          size: 40,
          color: Culori.accent,
        ),
        title: T(
          'Data viitoare cu ${app.numeBiometrie}?',
          marime: 20,
          gros: FontWeight.w700,
          aliniere: TextAlign.center,
        ),
        content: const T.mic(
          'PIN-ul rămâne criptat pe telefon.',
          aliniere: TextAlign.center,
        ),
        actions: [
          Row(
            children: [
              Expanded(
                child: Buton('Nu', onTap: () => Navigator.pop(c, false)),
              ),
              const SizedBox(width: 8),
              Expanded(
                child: Buton(
                  'Da',
                  principal: true,
                  onTap: () => Navigator.pop(c, true),
                ),
              ),
            ],
          ),
        ],
      ),
    );
    await app.raspundeBiometria(da ?? false);
    _intreb = false;
  }

  // telecomanda de pe usa se mai opreste singura si casa crede ca s-a inchis: aici ii
  // spunem cum e de fapt (nu misca usa, nu porneste alarma; tine pana transmite senzorul)
  Future<void> _corecteaza(
    AppState app,
    int i,
    String nume,
    bool deschis,
  ) async {
    HapticFeedback.mediumImpact();
    final da = await showDialog<bool>(
      context: context,
      builder: (c) => AlertDialog(
        backgroundColor: Culori.card,
        surfaceTintColor: Colors.transparent,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(coltCard),
        ),
        title: T(nume, marime: 20, gros: FontWeight.w700),
        content: T.mic(
          'De fapt e „${deschis ? 'închis' : 'deschis'}”? Ușa nu se mișcă, corectăm doar ce crede casa.',
        ),
        actions: [
          Row(
            children: [
              Expanded(
                child: Buton('Nu', onTap: () => Navigator.pop(c, false)),
              ),
              const SizedBox(width: 8),
              Expanded(
                child: Buton(
                  'Da',
                  principal: true,
                  onTap: () => Navigator.pop(c, true),
                ),
              ),
            ],
          ),
        ],
      ),
    );
    if (da == true) {
      await app.comanda(
        'USA $i ${deschis ? 0 : 1}',
        cheie: 'usa:$i',
        valoare: !deschis,
      );
    }
  }

  Future<void> _leaga(AppState app) async {
    final cod = await ceCodulPanoului(context);
    if (cod != null) await app.leagaTelefonul(cod);
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
    if (app.propunBiometria && !_intreb) {
      _intreb = true;
      WidgetsBinding.instance.addPostFrameCallback(
        (_) => _propuneBiometria(app),
      );
    }

    return Ecran(
      titlu: 'Securitate & Setări',
      copii: [
        // ---- alarma
        if (s != null)
          Bloc(
            pericol: s.alarma.pericol,
            lumina: s.alarma.cod >= 2 ? Culori.verde : null,
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  children: [
                    Iconita(
                      s.alarma.cod == 0 ? Icons.shield_outlined : Icons.shield,
                      culoare: s.alarma.pericol
                          ? Culori.rosu
                          : (s.alarma.cod == 0 ? Culori.text2 : Culori.verde),
                      aprinsa: s.alarma.cod >= 2,
                      marime: 52,
                    ),
                    const SizedBox(width: 14),
                    Expanded(
                      child: T.mediu(
                        s.alarma.stare,
                        culoare: s.alarma.pericol ? Culori.rosu : null,
                      ),
                    ),
                  ],
                ),
                if (_explicatieAlarma[s.alarma.cod.clamp(0, 5)].isNotEmpty) ...[
                  const SizedBox(height: 8),
                  T.mic(_explicatieAlarma[s.alarma.cod.clamp(0, 5)]),
                ],
                const SizedBox(height: 16),
                if (s.alarma.cod == 0)
                  Row(
                    children: [
                      Expanded(
                        child: Buton(
                          'Plec de acasă',
                          icon: Icons.logout,
                          principal: true,
                          onTap: () => app.comanda('ALARMA PLECAT'),
                        ),
                      ),
                      const SizedBox(width: 8),
                      Expanded(
                        child: Buton(
                          'Noapte',
                          icon: Icons.nightlight_outlined,
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
                      icon: app.amprentaPornita && app.areSalvatPin
                          ? (app.numeBiometrie == 'Face ID'
                                ? Icons.face_retouching_natural
                                : Icons.fingerprint)
                          : Icons.dialpad,
                      onTap: () => _dezarmeaza(app),
                    ),
                  ),
                // Face ID / Touch ID / amprenta: chiar langa butoanele alarmei, sa se vada
                if (app.amprentaDisponibila) ...[
                  const SizedBox(height: 16),
                  _RandSetare(
                    titlu: 'Dezarmare cu ${app.numeBiometrie}',
                    detaliu: !app.amprentaPornita
                        ? 'în loc de PIN'
                        : app.areSalvatPin
                        ? 'pornită'
                        : 'după încă o dezarmare cu PIN',
                    dreapta: Comutator(
                      eticheta: 'Dezarmare cu ${app.numeBiometrie}',
                      pornit: app.amprentaPornita,
                      onSchimba: (v) => app.schimbaSetari(amprenta: v),
                    ),
                  ),
                ],
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
                  info: 'Dacă o stare nu e cea reală, țineți apăsat pe ea.',
                ),
                for (var i = 0; i < s.alarma.senzori.length; i++)
                  Builder(
                    builder: (_) {
                      final cheie = 'usa:$i';
                      final deschis = app.arata(
                        cheie,
                        s.alarma.senzori[i].deschis,
                      );
                      return GestureDetector(
                        behavior: HitTestBehavior.opaque,
                        onLongPress: () => _corecteaza(
                          app,
                          i,
                          s.alarma.senzori[i].nume,
                          deschis,
                        ),
                        child: Container(
                          padding: const EdgeInsets.symmetric(vertical: 10),
                          decoration: BoxDecoration(
                            border: i > 0
                                ? Border(
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
                              ),
                            ],
                          ),
                        ),
                      );
                    },
                  ),
              ],
            ),
          ),

        // ---- panoul casei
        Bloc(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Eticheta(
                'Panoul casei',
                iconita: Icons.router_outlined,
                info: 'Adresa e pe ecranul panoului, la Setări.',
              ),
              TextField(
                controller: _adresa,
                keyboardType: TextInputType.url,
                autocorrect: false,
                onSubmitted: (_) => _conecteaza(app),
                style: fontCod(marime: 15, culoare: Culori.text),
                decoration: InputDecoration(
                  hintText: '192.168.1.42',
                  hintStyle: fontCod(marime: 15, culoare: Culori.text2),
                  isDense: true,
                  filled: true,
                  fillColor: Culori.moale,
                  prefixIcon: Icon(Icons.lan_outlined, color: Culori.text2),
                  contentPadding: const EdgeInsets.symmetric(
                    horizontal: 14,
                    vertical: 14,
                  ),
                  border: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(coltMic),
                    borderSide: BorderSide(color: Culori.linie),
                  ),
                  enabledBorder: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(coltMic),
                    borderSide: BorderSide(color: Culori.linie),
                  ),
                  focusedBorder: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(coltMic),
                    borderSide: BorderSide(color: Culori.accent, width: 1.5),
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
              // legat = are cheia casei: poate dezarma si prin internet (PIN-ul pleaca criptat)
              _RandSetare(
                titlu: app.legat
                    ? 'Telefonul e legat de casă'
                    : 'Leagă telefonul de casă',
                detaliu: !app.legat
                    ? 'ca să meargă și de pe date mobile'
                    : app.poatePrinInternet
                    ? 'merge și de pe date mobile'
                    : 'fără acces prin internet',
                dreapta: Buton(
                  app.legat ? 'Din nou' : 'Leagă',
                  onTap: () => _leaga(app),
                ),
              ),
              const SizedBox(height: 14),
              _RandSetare(
                titlu: 'Mod demo',
                detaliu: 'o seară simulată, fără panou',
                dreapta: Comutator(
                  eticheta: 'Mod demo',
                  pornit: app.demo,
                  onSchimba: (d) => app.schimbaSetari(demo: d),
                ),
              ),
              if (s != null && s.retea.ip.isNotEmpty) ...[
                const SizedBox(height: 10),
                T.cod(
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
              Eticheta('Cum se poartă casa', iconita: Icons.eco_outlined),
              _ModCasa(app),
              const SizedBox(height: 16),
              _RandSetare(
                titlu: 'Temperatura de confort',
                detaliu: 'când e cineva acasă',
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
            ],
          ),
        ),

        // ---- cum arata aplicatia
        Bloc(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Eticheta('Aspect', iconita: Icons.dark_mode_outlined),
              Alegere(
                optiuni: const {
                  'auto': 'Ca telefonul',
                  'zi': 'Zi',
                  'noapte': 'Noapte',
                },
                ales: app.tema,
                onAlege: (t) => app.schimbaSetari(tema: t),
              ),
            ],
          ),
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
    return Alegere(
      optiuni: {
        'smart': 'Smart${smart == true && app.asteapta('mod') ? '…' : ''}',
        'clasic': 'Clasic${smart == false && app.asteapta('mod') ? '…' : ''}',
      },
      ales: smart == null ? null : (smart ? 'smart' : 'clasic'),
      onAlege: (m) {
        if (smart == null) return;
        app.comanda(
          m == 'smart' ? 'MOD SMART' : 'MOD CLASIC',
          cheie: 'mod',
          valoare: m == 'smart',
        );
      },
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
        side: BorderSide(color: Culori.linie),
        backgroundColor: Culori.moale,
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
