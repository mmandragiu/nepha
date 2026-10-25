// tastatura_pin - PIN-ul alarmei. Il verifica Arduino, nu aplicatia: noi doar il trimitem.
import 'package:flutter/material.dart';

import '../theme/nepha_theme.dart';
import 'ui.dart';

// intoarce PIN-ul scris, sau null daca s-a renuntat
Future<String?> ceAPin(BuildContext context) => showDialog<String>(
  context: context,
  barrierColor: const Color(0x73171512),
  builder: (_) => const _TastaturaPin(),
);

class _TastaturaPin extends StatefulWidget {
  const _TastaturaPin();

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
      } else if (_pin.length < 4) {
        _pin += k;
      }
    });
  }

  @override
  Widget build(BuildContext context) => Dialog(
    backgroundColor: Culori.card,
    surfaceTintColor: Colors.transparent,
    shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
    child: Padding(
      padding: const EdgeInsets.all(20),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const T.mediu('PIN-ul alarmei'),
          const T.mic('Îl verifică Arduino, nu aplicația.'),
          const SizedBox(height: 14),
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              for (var i = 0; i < 4; i++)
                AnimatedContainer(
                  duration: const Duration(milliseconds: 150),
                  width: 14,
                  height: 14,
                  margin: const EdgeInsets.symmetric(horizontal: 6),
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    color: i < _pin.length ? Culori.text : Colors.transparent,
                    border: Border.all(
                      color: i < _pin.length ? Culori.text : Culori.linie,
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
                        borderRadius: BorderRadius.circular(12),
                        child: InkWell(
                          borderRadius: BorderRadius.circular(12),
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
                  'Dezarmează',
                  principal: true,
                  onTap: _pin.isEmpty
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
