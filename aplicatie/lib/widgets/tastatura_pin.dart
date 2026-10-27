// tastatura_pin - PIN-ul alarmei (il verifica Arduino, nu aplicatia: noi doar il trimitem)
// si codul de 6 cifre de pe panou, cand legam telefonul de casa.
import 'package:flutter/material.dart';

import '../theme/nepha_theme.dart';
import 'ui.dart';

// intoarce PIN-ul scris, sau null daca s-a renuntat
Future<String?> ceAPin(BuildContext context, {bool demo = false}) =>
    showDialog<String>(
      context: context,
      barrierColor: const Color(0x99050810),
      builder: (_) => _TastaturaPin(
        titlu: 'PIN-ul alarmei',
        detaliu: demo
            ? 'În modul demo, PIN-ul e 1234.'
            : 'Îl verifică Arduino, nu aplicația.',
        cifre: 4,
        buton: 'Dezarmează',
      ),
    );

// codul afisat pe panou la Setari -> Leaga telefonul (cifrele se vad, nu e un secret de tinut minte)
Future<String?> ceCodulPanoului(BuildContext context) => showDialog<String>(
  context: context,
  barrierColor: const Color(0x99050810),
  builder: (_) => const _TastaturaPin(
    titlu: 'Codul de pe panou',
    detaliu: 'Pe panou: Setări → „Leagă telefonul”. Codul e valabil 2 minute.',
    cifre: 6,
    buton: 'Leagă',
    aratCifrele: true,
  ),
);

class _TastaturaPin extends StatefulWidget {
  final String titlu, detaliu, buton;
  final int cifre;
  final bool aratCifrele;
  const _TastaturaPin({
    required this.titlu,
    required this.detaliu,
    required this.cifre,
    required this.buton,
    this.aratCifrele = false,
  });

  @override
  State<_TastaturaPin> createState() => _TastaturaPinState();
}

class _TastaturaPinState extends State<_TastaturaPin> {
  String _pin = '';
  static const _taste = [
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '',
    '0',
    '⌫',
  ];

  void _apasa(String k) {
    atingere();
    setState(() {
      if (k == '⌫') {
        if (_pin.isNotEmpty) _pin = _pin.substring(0, _pin.length - 1);
      } else if (_pin.length < widget.cifre) {
        _pin += k;
      }
    });
  }

  @override
  Widget build(BuildContext context) => Dialog(
    backgroundColor: Culori.card,
    surfaceTintColor: Colors.transparent,
    shape: RoundedRectangleBorder(
      borderRadius: BorderRadius.circular(coltCard),
      side: BorderSide(color: Culori.linie),
    ),
    child: Padding(
      padding: const EdgeInsets.all(20),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          T.mediu(widget.titlu),
          T.mic(widget.detaliu),
          const SizedBox(height: 14),
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              for (var i = 0; i < widget.cifre; i++)
                if (widget.aratCifrele)
                  Container(
                    width: 30,
                    height: 40,
                    margin: const EdgeInsets.symmetric(horizontal: 3),
                    alignment: Alignment.center,
                    decoration: BoxDecoration(
                      color: Culori.moale,
                      borderRadius: BorderRadius.circular(10),
                      border: Border.all(
                        color: i == _pin.length ? Culori.accent : Culori.linie,
                      ),
                    ),
                    child: T.cod(
                      i < _pin.length ? _pin[i] : '',
                      marime: 20,
                      culoare: Culori.text,
                    ),
                  )
                else
                  AnimatedContainer(
                    duration: const Duration(milliseconds: 150),
                    width: 14,
                    height: 14,
                    margin: const EdgeInsets.symmetric(horizontal: 6),
                    decoration: BoxDecoration(
                      shape: BoxShape.circle,
                      color: i < _pin.length
                          ? Culori.accent
                          : Colors.transparent,
                      border: Border.all(
                        color: i < _pin.length ? Culori.accent : Culori.linie,
                        width: 2,
                      ),
                    ),
                  ),
            ],
          ),
          const SizedBox(height: 14),
          GridView.count(
            crossAxisCount: 3,
            shrinkWrap: true,
            mainAxisSpacing: 8,
            crossAxisSpacing: 8,
            childAspectRatio: 1.7,
            physics: const NeverScrollableScrollPhysics(),
            children: [
              for (final k in _taste)
                k.isEmpty
                    ? const SizedBox.shrink()
                    : Material(
                        color: Culori.moale,
                        borderRadius: BorderRadius.circular(coltMic),
                        child: InkWell(
                          borderRadius: BorderRadius.circular(coltMic),
                          onTap: () => _apasa(k),
                          child: Center(
                            child: T(k, marime: 20, gros: FontWeight.w700),
                          ),
                        ),
                      ),
            ],
          ),
          const SizedBox(height: 16),
          Row(
            children: [
              Expanded(
                child: Buton('Renunț', onTap: () => Navigator.pop(context)),
              ),
              const SizedBox(width: 8),
              Expanded(
                child: Buton(
                  widget.buton,
                  principal: true,
                  onTap:
                      _pin.isEmpty ||
                          (widget.aratCifrele && _pin.length < widget.cifre)
                      ? null
                      : () => Navigator.pop(context, _pin),
                ),
              ),
            ],
          ),
        ],
      ),
    ),
  );
}
