// ui - bucatile din care facem ecranele (acelasi aspect ca pagina casei)
import 'dart:math';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';

import '../providers/app_state.dart';
import '../theme/nepha_theme.dart';

// ---------------------------------------------------------------- numere scrise ca la noi

// 21.4 -> "21,4"; 16200 -> "16.200"
String nr(num? x, [int zecimale = 1]) {
  if (x == null || x.isNaN) return '–';
  final t = x.abs().toStringAsFixed(zecimale).split('.');
  final intreg = t[0].replaceAllMapped(
    RegExp(r'\B(?=(\d{3})+(?!\d))'),
    (_) => '.',
  );
  return '${x < 0 ? '−' : ''}$intreg${t.length > 1 ? ',${t[1]}' : ''}';
}

String grade(num? x) => x == null ? '–' : '${nr(x)}°';

void atingere() => HapticFeedback.selectionClick();

// ---------------------------------------------------------------- texte

class T extends StatelessWidget {
  final String text;
  final double marime;
  final FontWeight gros;
  final Color? culoare;
  final TextAlign? aliniere;
  final int? randuri;
  final bool _mic; // textul mic e gri, daca nu zicem altfel
  const T(
    this.text, {
    super.key,
    this.marime = 15,
    this.gros = FontWeight.w400,
    this.culoare,
    this.aliniere,
    this.randuri,
  }) : _mic = false;

  const T.mic(this.text, {super.key, this.culoare, this.aliniere, this.randuri})
    : marime = 13,
      gros = FontWeight.w400,
      _mic = true;

  const T.mare(
    this.text, {
    super.key,
    this.culoare,
    this.aliniere,
    this.randuri,
  }) : marime = 44,
       gros = FontWeight.w800,
       _mic = false;

  const T.mediu(
    this.text, {
    super.key,
    this.culoare,
    this.aliniere,
    this.randuri,
  }) : marime = 24,
       gros = FontWeight.w700,
       _mic = false;

  @override
  Widget build(BuildContext context) => Text(
    text,
    textAlign: aliniere,
    maxLines: randuri,
    overflow: randuri != null ? TextOverflow.ellipsis : null,
    style: Theme.of(context).textTheme.bodyMedium!.copyWith(
      fontSize: marime,
      fontWeight: gros,
      color: culoare ?? (_mic ? Culori.text2 : Culori.text),
      height: marime > 30 ? 1.1 : 1.35,
      letterSpacing: marime > 30 ? -1 : 0,
    ),
  );
}

// ---------------------------------------------------------------- ecran

class Ecran extends StatelessWidget {
  final String titlu;
  final String? subtitlu;
  final List<Widget> copii;
  const Ecran({
    super.key,
    required this.titlu,
    this.subtitlu,
    required this.copii,
  });

  @override
  Widget build(BuildContext context) {
    final demo = context.select<AppState, bool>(
      (s) => s.conexiune == Conexiune.demo,
    );
    return SafeArea(
      bottom: false,
      child: RefreshIndicator(
        color: Culori.verde,
        onRefresh: () async => context.read<AppState>().improspateaza(),
        child: ListView(
          padding: const EdgeInsets.fromLTRB(16, 16, 16, 32),
          children: [
            Row(
              children: [
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      T(titlu, marime: 28, gros: FontWeight.w800),
                      if (subtitlu != null) T.mic(subtitlu!),
                    ],
                  ),
                ),
                const IndicatorConexiune(),
              ],
            ),
            const SizedBox(height: 12),
            if (demo) ...[
              Container(
                padding: const EdgeInsets.symmetric(
                  horizontal: 14,
                  vertical: 10,
                ),
                decoration: BoxDecoration(
                  color: Culori.soareD,
                  borderRadius: BorderRadius.circular(coltCard),
                ),
                child: T.mic(
                  'Mod demo: vedeți o seară simulată. Adresa panoului se pune la Securitate & Setări.',
                  culoare: Culori.text,
                ),
              ),
              const SizedBox(height: 12),
            ],
            for (final c in copii) ...[c, const SizedBox(height: 12)],
          ],
        ),
      ),
    );
  }
}

// ---------------------------------------------------------------- card

class Bloc extends StatefulWidget {
  final Widget child;
  final VoidCallback? onTap;
  final bool pericol; // alarma suna: chenar rosu
  const Bloc({
    super.key,
    required this.child,
    this.onTap,
    this.pericol = false,
  });

  @override
  State<Bloc> createState() => _BlocState();
}

class _BlocState extends State<Bloc> {
  bool _apasat = false;

  @override
  Widget build(BuildContext context) {
    final card = AnimatedContainer(
      duration: const Duration(milliseconds: 300),
      curve: usor,
      transform: Matrix4.translationValues(
        0,
        _apasat ? -2 : 0,
        0,
      ), // se ridica putin la atingere
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: widget.pericol ? Culori.cardPericol : Culori.card,
        borderRadius: BorderRadius.circular(coltCard),
        border: Border.all(
          color: widget.pericol ? Culori.rosu : Culori.linie,
          width: widget.pericol ? 1.5 : 0.6,
        ),
        boxShadow: umbraCard,
      ),
      child: widget.child,
    );
    if (widget.onTap == null) return card;
    return GestureDetector(
      onTapDown: (_) => setState(() => _apasat = true),
      onTapCancel: () => setState(() => _apasat = false),
      onTapUp: (_) => setState(() => _apasat = false),
      onTap: () {
        atingere();
        widget.onTap!();
      },
      child: card,
    );
  }
}

class Eticheta extends StatelessWidget {
  final IconData? iconita;
  final Color? culoare;
  final String text;
  final Widget? dreapta;
  const Eticheta(
    this.text, {
    super.key,
    this.iconita,
    this.culoare,
    this.dreapta,
  });

  @override
  Widget build(BuildContext context) => Padding(
    padding: const EdgeInsets.only(bottom: 10),
    child: SizedBox(
      height: 26,
      child: Row(
        children: [
          if (iconita != null) ...[
            Icon(iconita, size: 18, color: culoare ?? Culori.text2),
            const SizedBox(width: 8),
          ],
          Expanded(
            child: T(
              text,
              marime: 14,
              gros: FontWeight.w700,
              culoare: Culori.text2,
              randuri: 1,
            ),
          ),
          ?dreapta,
        ],
      ),
    ),
  );
}

// ---------------------------------------------------------------- pastila

enum Ton { gri, verde, soare, apa, cald, rosu }

Map<Ton, (Color, Color)> get _tonuri => {
  Ton.gri: (Culori.moale, Culori.text2),
  Ton.verde: (Culori.verdeD, Culori.verde),
  Ton.soare: (Culori.soareD, Culori.soare),
  Ton.apa: (Culori.apaD, Culori.apa),
  Ton.cald: (Culori.caldD, Culori.cald),
  Ton.rosu: (Culori.rosuD, Culori.rosu),
};

class Pastila extends StatelessWidget {
  final String text;
  final Ton ton;
  final VoidCallback? onTap;
  final bool asteapta;
  const Pastila(
    this.text, {
    super.key,
    this.ton = Ton.gri,
    this.onTap,
    this.asteapta = false,
  });

  @override
  Widget build(BuildContext context) {
    final (fundal, culoare) = _tonuri[ton]!;
    final p = AnimatedContainer(
      duration: const Duration(milliseconds: 200),
      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
      decoration: BoxDecoration(
        color: fundal,
        borderRadius: BorderRadius.circular(99),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          if (asteapta) ...[
            SizedBox(
              width: 10,
              height: 10,
              child: CircularProgressIndicator(
                strokeWidth: 1.6,
                color: culoare,
              ),
            ),
            const SizedBox(width: 6),
          ],
          T(text, marime: 12, gros: FontWeight.w700, culoare: culoare),
        ],
      ),
    );
    if (onTap == null) return p;
    return InkWell(
      borderRadius: BorderRadius.circular(99),
      onTap: () {
        atingere();
        onTap!();
      },
      child: p,
    );
  }
}

// ---------------------------------------------------------------- buton

class Buton extends StatelessWidget {
  final String text;
  final VoidCallback? onTap;
  final bool principal;
  const Buton(this.text, {super.key, this.onTap, this.principal = false});

  @override
  Widget build(BuildContext context) {
    final stil = ButtonStyle(
      padding: const WidgetStatePropertyAll(
        EdgeInsets.symmetric(vertical: 13, horizontal: 18),
      ),
      shape: WidgetStatePropertyAll(
        RoundedRectangleBorder(borderRadius: BorderRadius.circular(99)),
      ),
      textStyle: WidgetStatePropertyAll(
        Theme.of(context).textTheme.bodyMedium!.copyWith(
          fontSize: 15,
          fontWeight: FontWeight.w700,
        ),
      ),
    );
    final apasa = onTap == null
        ? null
        : () {
            atingere();
            onTap!();
          };
    return principal
        ? FilledButton(
            onPressed: apasa,
            style: stil.copyWith(
              backgroundColor: WidgetStateProperty.resolveWith(
                (s) => s.contains(WidgetState.disabled)
                    ? Culori.verde.withValues(alpha: 0.45)
                    : Culori.verde,
              ),
            ),
            child: Text(text, style: const TextStyle(color: Colors.white)),
          )
        : OutlinedButton(
            onPressed: apasa,
            style: stil.copyWith(
              side: WidgetStatePropertyAll(BorderSide(color: Culori.linie)),
              backgroundColor: WidgetStatePropertyAll(Culori.card),
              foregroundColor: WidgetStatePropertyAll(Culori.text),
            ),
            child: Text(text),
          );
  }
}

// ---------------------------------------------------------------- comutator

// se muta pe loc; cat asteptam raspunsul panoului, arata un cerc care se invarte
class Comutator extends StatelessWidget {
  final bool pornit;
  final ValueChanged<bool> onSchimba;
  final bool asteapta;
  final Color? culoare;
  final String eticheta;
  const Comutator({
    super.key,
    required this.pornit,
    required this.onSchimba,
    this.asteapta = false,
    this.culoare,
    required this.eticheta,
  });

  @override
  Widget build(BuildContext context) => Semantics(
    label: eticheta,
    toggled: pornit,
    child: GestureDetector(
      onTap: () {
        atingere();
        onSchimba(!pornit);
      },
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 200),
        curve: usor,
        width: 46,
        height: 28,
        padding: const EdgeInsets.all(3),
        decoration: BoxDecoration(
          color: pornit ? (culoare ?? Culori.soare) : Culori.linie,
          borderRadius: BorderRadius.circular(99),
        ),
        child: AnimatedAlign(
          duration: const Duration(milliseconds: 200),
          curve: usor,
          alignment: pornit ? Alignment.centerRight : Alignment.centerLeft,
          child: Container(
            width: 22,
            height: 22,
            decoration: BoxDecoration(
              color: Culori.card,
              shape: BoxShape.circle,
              boxShadow: [
                BoxShadow(
                  color: Color(0x33000000),
                  blurRadius: 3,
                  offset: Offset(0, 1),
                ),
              ],
            ),
            padding: const EdgeInsets.all(5),
            child: asteapta
                ? CircularProgressIndicator(
                    strokeWidth: 1.6,
                    color: Culori.text2,
                  )
                : null,
          ),
        ),
      ),
    ),
  );
}

// ---------------------------------------------------------------- bara

// bara aluneca in 0,5 s, cu aceeasi curba ca pe panou
class Bara extends StatelessWidget {
  final double procent;
  final Color? culoare;
  final Color? fundal;
  const Bara(this.procent, {super.key, this.culoare, this.fundal});

  @override
  Widget build(BuildContext context) => ClipRRect(
    borderRadius: BorderRadius.circular(99),
    child: Container(
      height: 8,
      color: fundal ?? Culori.linie,
      alignment: Alignment.centerLeft,
      child: TweenAnimationBuilder<double>(
        tween: Tween(end: (procent / 100).clamp(0, 1).toDouble()),
        duration: const Duration(milliseconds: 500),
        curve: usor,
        builder: (_, v, _) => FractionallySizedBox(
          widthFactor: v,
          child: Container(
            decoration: BoxDecoration(
              color: culoare ?? Culori.verde,
              borderRadius: BorderRadius.circular(99),
            ),
          ),
        ),
      ),
    ),
  );
}

// ---------------------------------------------------------------- punct care pulseaza

class Punct extends StatefulWidget {
  final Color culoare;
  final bool pulseaza;
  const Punct({super.key, required this.culoare, this.pulseaza = false});

  @override
  State<Punct> createState() => _PunctState();
}

class _PunctState extends State<Punct> with SingleTickerProviderStateMixin {
  late final _a = AnimationController(
    vsync: this,
    duration: const Duration(seconds: 2),
  );

  @override
  void initState() {
    super.initState();
    if (widget.pulseaza) _a.repeat();
  }

  @override
  void didUpdateWidget(Punct vechi) {
    super.didUpdateWidget(vechi);
    if (widget.pulseaza && !_a.isAnimating) _a.repeat();
    if (!widget.pulseaza) _a.stop();
  }

  @override
  void dispose() {
    _a.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) => SizedBox(
    width: 8,
    height: 8,
    child: AnimatedBuilder(
      animation: _a,
      builder: (_, _) {
        final t = usor.transform(_a.value);
        return Stack(
          clipBehavior: Clip.none,
          alignment: Alignment.center,
          children: [
            if (widget.pulseaza)
              Transform.scale(
                scale: 1 + 1.4 * min(1, t / 0.7),
                child: Opacity(
                  opacity: (0.45 * (1 - min(1, t / 0.7)))
                      .clamp(0, 1)
                      .toDouble(),
                  child: Container(
                    decoration: BoxDecoration(
                      color: widget.culoare,
                      shape: BoxShape.circle,
                    ),
                  ),
                ),
              ),
            Container(
              decoration: BoxDecoration(
                color: widget.culoare,
                shape: BoxShape.circle,
              ),
            ),
          ],
        );
      },
    ),
  );
}

// ---------------------------------------------------------------- legatura cu panoul

class IndicatorConexiune extends StatelessWidget {
  const IndicatorConexiune({super.key});

  @override
  Widget build(BuildContext context) {
    final c = context.select<AppState, Conexiune>((s) => s.conexiune);
    final pulseaza = c == Conexiune.conectat || c == Conexiune.seConecteaza;
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
      decoration: BoxDecoration(
        color: Culori.card,
        borderRadius: BorderRadius.circular(99),
        border: Border.all(color: Culori.linie, width: 0.6),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Punct(culoare: c.culoare, pulseaza: pulseaza),
          const SizedBox(width: 8),
          T.mic(c.text),
        ],
      ),
    );
  }
}

// ---------------------------------------------------------------- inelul gradinii

class Inel extends StatelessWidget {
  final int procent;
  final double marime;
  const Inel(this.procent, {super.key, this.marime = 110});

  @override
  Widget build(BuildContext context) => SizedBox(
    width: marime,
    height: marime,
    child: TweenAnimationBuilder<double>(
      tween: Tween(end: procent.clamp(0, 100) / 100),
      duration: const Duration(milliseconds: 500),
      curve: usor,
      builder: (_, v, _) => CustomPaint(
        painter: _InelPictor(v),
        child: Center(
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [T.mediu('$procent%'), const T.mic('umiditate')],
          ),
        ),
      ),
    ),
  );
}

class _InelPictor extends CustomPainter {
  final double p;
  _InelPictor(this.p);

  @override
  void paint(Canvas canvas, Size size) {
    const g = 12.0;
    final r = Rect.fromLTWH(g / 2, g / 2, size.width - g, size.height - g);
    final linie = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = g;
    canvas.drawArc(r, 0, 2 * pi, false, linie..color = Culori.moale);
    canvas.drawArc(r, -pi / 2, 2 * pi * p, false, linie..color = Culori.apa);
  }

  @override
  bool shouldRepaint(_InelPictor vechi) => vechi.p != p;
}
