// ui - bucatile din care facem ecranele: carduri, iconite rotunde, pastile, comutatoare,
// grafice mici si "sticla" pentru ce pluteste peste ecran
import 'dart:math';
import 'dart:ui';

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
  final bool _cod; // scris de "inginer": adrese, protocoale, milisecunde
  const T(
    this.text, {
    super.key,
    this.marime = 15,
    this.gros = FontWeight.w500,
    this.culoare,
    this.aliniere,
    this.randuri,
  }) : _mic = false,
       _cod = false;

  const T.mic(this.text, {super.key, this.culoare, this.aliniere, this.randuri})
    : marime = 13,
      gros = FontWeight.w500,
      _mic = true,
      _cod = false;

  // cifrele mari se scriu subtire (ca pe un ceas), nu ingrosat
  const T.mare(
    this.text, {
    super.key,
    this.marime = 44,
    this.culoare,
    this.aliniere,
    this.randuri,
  }) : gros = FontWeight.w300,
       _mic = false,
       _cod = false;

  const T.mediu(
    this.text, {
    super.key,
    this.culoare,
    this.aliniere,
    this.randuri,
  }) : marime = 22,
       gros = FontWeight.w700,
       _mic = false,
       _cod = false;

  const T.cod(
    this.text, {
    super.key,
    this.marime = 11,
    this.culoare,
    this.aliniere,
    this.randuri,
  }) : gros = FontWeight.w500,
       _mic = true,
       _cod = true;

  @override
  Widget build(BuildContext context) {
    final culoareText = culoare ?? (_mic ? Culori.text2 : Culori.text);
    final stil = _cod
        ? fontCod(marime: marime, gros: gros, culoare: culoareText)
        : fontText(marime: marime, gros: gros, culoare: culoareText);
    return Text(
      text,
      textAlign: aliniere,
      maxLines: randuri,
      overflow: randuri != null ? TextOverflow.ellipsis : null,
      style: stil.copyWith(
        height: marime > 30 ? 1.05 : 1.3,
        letterSpacing: _cod
            ? 0
            : (marime > 30 ? -0.8 : (marime > 20 ? -0.2 : 0)),
        fontFeatures: const [FontFeature.tabularFigures()],
      ),
    );
  }
}

// ---------------------------------------------------------------- ecran

class Ecran extends StatelessWidget {
  final String titlu;
  final String?
  titluTare; // a doua parte a titlului, ingrosata ("Casa ta, *pe scurt*")
  final String? subtitlu;
  final List<Widget> copii;
  const Ecran({
    super.key,
    required this.titlu,
    this.titluTare,
    this.subtitlu,
    required this.copii,
  });

  @override
  Widget build(BuildContext context) {
    final demo = context.select<AppState, bool>(
      (s) => s.conexiune == Conexiune.demo,
    );
    final latime = MediaQuery.sizeOf(context).width;
    final lateral = latime > 752 ? (latime - 720) / 2 : 16.0;
    final stilTitlu = fontText(
      marime: 34,
      gros: FontWeight.w400,
      culoare: Culori.text,
    ).copyWith(height: 1.08, letterSpacing: -0.4);
    return SafeArea(
      bottom: false,
      child: RefreshIndicator(
        color: Culori.accent,
        backgroundColor: Culori.card,
        onRefresh: () async => context.read<AppState>().improspateaza(),
        child: ListView(
          // jos lasam loc pentru bara care pluteste peste ecran; pe tableta nu intindem
          // cardurile pe toata latimea, le tinem pe mijloc
          padding: EdgeInsets.fromLTRB(lateral, 12, lateral, 120),
          children: [
            Row(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      if (subtitlu != null) ...[
                        T.mic(subtitlu!),
                        const SizedBox(height: 4),
                      ],
                      Text.rich(
                        TextSpan(
                          style: stilTitlu,
                          children: [
                            TextSpan(text: titlu),
                            if (titluTare != null)
                              TextSpan(
                                text: titluTare,
                                style: stilTitlu.copyWith(
                                  fontWeight: FontWeight.w700,
                                ),
                              ),
                          ],
                        ),
                      ),
                    ],
                  ),
                ),
                const SizedBox(width: 10),
                const IndicatorConexiune(),
              ],
            ),
            const SizedBox(height: 18),
            if (demo) ...[
              Container(
                padding: const EdgeInsets.fromLTRB(14, 10, 14, 10),
                decoration: BoxDecoration(
                  color: Culori.soareD,
                  borderRadius: BorderRadius.circular(coltMic),
                ),
                child: Row(
                  children: [
                    Icon(Icons.science_outlined, size: 18, color: Culori.soare),
                    const SizedBox(width: 10),
                    Expanded(
                      child: T.mic(
                        'Mod demo · o seară simulată',
                        culoare: Culori.text,
                      ),
                    ),
                  ],
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

// titlul unei sectiuni, intre carduri
class Sectiune extends StatelessWidget {
  final String text;
  final String? info;
  final Widget? dreapta;
  const Sectiune(this.text, {super.key, this.info, this.dreapta});

  @override
  Widget build(BuildContext context) => Padding(
    padding: const EdgeInsets.fromLTRB(4, 12, 4, 0),
    child: Row(
      children: [
        T(text, marime: 20, gros: FontWeight.w700),
        if (info != null) Info(info!),
        Expanded(
          child: Align(alignment: Alignment.centerRight, child: dreapta),
        ),
      ],
    ),
  );
}

// ---------------------------------------------------------------- card

class Bloc extends StatefulWidget {
  final Widget child;
  final VoidCallback? onTap;
  final VoidCallback? onLongPress;
  final bool pericol; // alarma suna: chenar rosu
  final Color?
  lumina; // ceva pornit: cardul capata o nuanta foarte slaba din culoarea asta
  final EdgeInsets padding;
  const Bloc({
    super.key,
    required this.child,
    this.onTap,
    this.onLongPress,
    this.pericol = false,
    this.lumina,
    this.padding = const EdgeInsets.all(18),
  });

  @override
  State<Bloc> createState() => _BlocState();
}

class _BlocState extends State<Bloc> {
  bool _apasat = false;

  @override
  Widget build(BuildContext context) {
    final card = AnimatedScale(
      scale: _apasat ? 0.98 : 1, // se adanceste putin sub deget
      duration: const Duration(milliseconds: 160),
      curve: usor,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 300),
        curve: usor,
        padding: widget.padding,
        decoration: decorCard(lumina: widget.lumina, pericol: widget.pericol),
        child: widget.child,
      ),
    );
    if (widget.onTap == null && widget.onLongPress == null) return card;
    return GestureDetector(
      onTapDown: (_) => setState(() => _apasat = true),
      onTapCancel: () => setState(() => _apasat = false),
      onTapUp: (_) => setState(() => _apasat = false),
      onLongPressEnd: (_) => setState(() => _apasat = false),
      onTap: widget.onTap == null
          ? null
          : () {
              atingere();
              widget.onTap!();
            },
      onLongPress: widget.onLongPress == null
          ? null
          : () {
              HapticFeedback.mediumImpact();
              setState(() => _apasat = false);
              widget.onLongPress!();
            },
      child: card,
    );
  }
}

BoxDecoration decorCard({Color? lumina, bool pericol = false}) => BoxDecoration(
  color: pericol
      ? Culori.cardPericol
      : lumina != null
      ? Color.alphaBlend(
          lumina.withValues(alpha: Culori.intunecat ? 0.10 : 0.06),
          Culori.card,
        )
      : Culori.card,
  borderRadius: BorderRadius.circular(coltCard),
  border: pericol ? Border.all(color: Culori.rosu, width: 1.5) : null,
  boxShadow: umbraCard,
);

// ---------------------------------------------------------------- sticla

// "Liquid Glass" facut de noi: ce e dedesubt se vede incetosat si putin mai viu, iar marginea
// prinde lumina din stanga-sus. Doar pentru ce pluteste peste continut (bara de jos, butoanele
// de sus), cum recomanda Apple; cardurile raman plate. Merge la fel pe iPhone si pe Android.
class Sticla extends StatelessWidget {
  final Widget child;
  final double colt;
  final EdgeInsets padding;
  final bool fumurie; // sticla inchisa la culoare (bara de jos), si ziua
  const Sticla({
    super.key,
    required this.child,
    this.colt = 99,
    this.padding = EdgeInsets.zero,
    this.fumurie = false,
  });

  // culorile de sub sticla ies putin mai vii (ca la Apple), nu spalacite de blur
  static const _maiViu = ColorFilter.matrix([
    1.4, -0.3, -0.1, 0, 0, //
    -0.1, 1.3, -0.2, 0, 0, //
    -0.1, -0.3, 1.4, 0, 0, //
    0, 0, 0, 1, 0,
  ]);

  @override
  Widget build(BuildContext context) {
    final inchisa = fumurie || Culori.intunecat;
    return ClipRRect(
      borderRadius: BorderRadius.circular(colt),
      child: BackdropFilter(
        filter: ImageFilter.compose(
          outer: _maiViu,
          inner: ImageFilter.blur(sigmaX: 22, sigmaY: 22),
        ),
        child: CustomPaint(
          foregroundPainter: _Margine(colt, inchisa),
          child: Container(
            padding: padding,
            color: inchisa
                ? const Color(0xFF1C1C1E).withValues(alpha: 0.74)
                : Colors.white.withValues(alpha: 0.68),
            child: child,
          ),
        ),
      ),
    );
  }
}

class _Margine extends CustomPainter {
  final double colt;
  final bool inchisa;
  _Margine(this.colt, this.inchisa);

  @override
  void paint(Canvas canvas, Size s) {
    final r = RRect.fromRectAndRadius(
      (Offset.zero & s).deflate(0.5),
      Radius.circular(min(colt, s.shortestSide / 2)),
    );
    // un fir subtire care se vede pe orice fundal...
    canvas.drawRRect(
      r,
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1
        ..color = inchisa ? const Color(0x1AFFFFFF) : const Color(0x12000000),
    );
    // ...si lumina care cade din stanga-sus pe margine
    canvas.drawRRect(
      r,
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1
        ..shader = LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [
            Colors.white.withValues(alpha: inchisa ? 0.3 : 0.9),
            Colors.white.withValues(alpha: 0),
            Colors.white.withValues(alpha: inchisa ? 0.1 : 0.5),
          ],
          stops: const [0, 0.5, 1],
        ).createShader(Offset.zero & s),
    );
  }

  @override
  bool shouldRepaint(_Margine vechi) =>
      vechi.colt != colt || vechi.inchisa != inchisa;
}

// ---------------------------------------------------------------- iconita rotunda

// stinsa: cerc gri cu iconita inchisa. Aprinsa: cerc plin in culoarea ei, iconita alba.
class Iconita extends StatelessWidget {
  final IconData icon;
  final Color? culoare;
  final bool aprinsa;
  final double marime;
  const Iconita(
    this.icon, {
    super.key,
    this.culoare,
    this.aprinsa = false,
    this.marime = 44,
  });

  @override
  Widget build(BuildContext context) => AnimatedContainer(
    duration: const Duration(milliseconds: 250),
    width: marime,
    height: marime,
    decoration: BoxDecoration(
      color: aprinsa ? (culoare ?? Culori.accent) : Culori.moale,
      shape: BoxShape.circle,
    ),
    child: Icon(
      icon,
      size: marime * 0.46,
      color: aprinsa ? Colors.white : Culori.text,
    ),
  );
}

class Eticheta extends StatelessWidget {
  final IconData? iconita;
  final String text;
  final String? info;
  final Widget? dreapta;
  const Eticheta(this.text, {super.key, this.iconita, this.info, this.dreapta});

  @override
  Widget build(BuildContext context) => Padding(
    padding: const EdgeInsets.only(bottom: 14),
    child: SizedBox(
      height: 36,
      child: Row(
        children: [
          if (iconita != null) ...[
            Iconita(iconita!, marime: 36),
            const SizedBox(width: 10),
          ],
          Expanded(
            child: Row(
              children: [
                Flexible(
                  child: T(text, marime: 16, gros: FontWeight.w700, randuri: 1),
                ),
                if (info != null) Info(info!),
              ],
            ),
          ),
          ?dreapta,
        ],
      ),
    ),
  );
}

// un (i) mic langa titlu: explicatia apare doar daca o ceri (atingi iconita)
class Info extends StatelessWidget {
  final String text;
  const Info(this.text, {super.key});

  @override
  Widget build(BuildContext context) => Tooltip(
    message: text,
    triggerMode: TooltipTriggerMode.tap,
    showDuration: const Duration(seconds: 5),
    margin: const EdgeInsets.symmetric(horizontal: 32),
    padding: const EdgeInsets.fromLTRB(14, 10, 14, 10),
    decoration: BoxDecoration(
      color: Culori.text,
      borderRadius: BorderRadius.circular(14),
    ),
    textStyle: fontText(marime: 13, culoare: Culori.fundal),
    child: Padding(
      padding: const EdgeInsets.all(6),
      child: Icon(Icons.info_outline_rounded, size: 17, color: Culori.text2),
    ),
  );
}

// ---------------------------------------------------------------- alegere

// cateva butoane lipite, unul singur ales (ca la Smart / Clasic)
class Alegere extends StatelessWidget {
  final Map<String, String> optiuni; // cheie -> ce scrie pe buton
  final String? ales;
  final ValueChanged<String> onAlege;
  const Alegere({
    super.key,
    required this.optiuni,
    required this.ales,
    required this.onAlege,
  });

  @override
  Widget build(BuildContext context) => Container(
    padding: const EdgeInsets.all(4),
    decoration: BoxDecoration(
      color: Culori.moale,
      borderRadius: BorderRadius.circular(99),
    ),
    child: Row(
      children: [
        for (final o in optiuni.entries)
          Expanded(
            child: GestureDetector(
              onTap: () {
                if (o.key == ales) return;
                atingere();
                onAlege(o.key);
              },
              child: AnimatedContainer(
                duration: const Duration(milliseconds: 220),
                curve: usor,
                padding: const EdgeInsets.symmetric(vertical: 9),
                decoration: BoxDecoration(
                  color: o.key == ales ? Culori.accent : Colors.transparent,
                  borderRadius: BorderRadius.circular(99),
                ),
                child: T(
                  o.value,
                  marime: 14,
                  gros: FontWeight.w600,
                  culoare: o.key == ales ? Culori.peAccent : Culori.text2,
                  aliniere: TextAlign.center,
                  randuri: 1,
                ),
              ),
            ),
          ),
      ],
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
  final IconData? icon;
  const Pastila(
    this.text, {
    super.key,
    this.ton = Ton.gri,
    this.onTap,
    this.asteapta = false,
    this.icon,
  });

  @override
  Widget build(BuildContext context) {
    final (fundal, culoare) = _tonuri[ton]!;
    final scris = ton == Ton.gri ? Culori.text : culoare;
    final p = AnimatedContainer(
      duration: const Duration(milliseconds: 200),
      padding: const EdgeInsets.symmetric(horizontal: 11, vertical: 5),
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
              child: CircularProgressIndicator(strokeWidth: 1.6, color: scris),
            ),
            const SizedBox(width: 6),
          ] else if (icon != null) ...[
            Icon(icon, size: 13, color: scris),
            const SizedBox(width: 5),
          ],
          Flexible(
            child: T(
              text,
              marime: 12,
              gros: FontWeight.w600,
              culoare: scris,
              randuri: 1,
            ),
          ),
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

// ---------------------------------------------------------------- butoane

class Buton extends StatelessWidget {
  final String text;
  final VoidCallback? onTap;
  final bool principal;
  final IconData? icon;
  const Buton(
    this.text, {
    super.key,
    this.onTap,
    this.principal = false,
    this.icon,
  });

  @override
  Widget build(BuildContext context) {
    final activ = onTap != null;
    final scris = principal
        ? Culori.peAccent
        : (activ ? Culori.text : Culori.text2);
    return GestureDetector(
      onTap: !activ
          ? null
          : () {
              atingere();
              onTap!();
            },
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 200),
        padding: const EdgeInsets.symmetric(vertical: 15, horizontal: 18),
        decoration: BoxDecoration(
          color: principal
              ? Culori.accent.withValues(alpha: activ ? 1 : 0.4)
              : Culori.moale,
          borderRadius: BorderRadius.circular(99),
        ),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            if (icon != null) ...[
              Icon(icon, size: 18, color: scris),
              const SizedBox(width: 8),
            ],
            Flexible(
              child: T(
                text,
                gros: FontWeight.w700,
                culoare: scris,
                aliniere: TextAlign.center,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

// buton rotund doar cu iconita (inchide, sageata)
class ButonRotund extends StatelessWidget {
  final IconData icon;
  final VoidCallback onTap;
  final double marime;
  final String eticheta;
  const ButonRotund(
    this.icon, {
    super.key,
    required this.onTap,
    required this.eticheta,
    this.marime = 40,
  });

  @override
  Widget build(BuildContext context) => Semantics(
    button: true,
    label: eticheta,
    child: GestureDetector(
      onTap: () {
        atingere();
        onTap();
      },
      child: Container(
        width: marime,
        height: marime,
        decoration: BoxDecoration(color: Culori.moale, shape: BoxShape.circle),
        child: Icon(icon, size: marime * 0.45, color: Culori.text),
      ),
    ),
  );
}

// ---------------------------------------------------------------- comutator

// ca un buton de pornire: bila alba are semnul de "power", portocaliu cand e pornit.
// Se muta pe loc; cat asteptam raspunsul panoului, bila arata un cerc care se invarte
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
  Widget build(BuildContext context) {
    final c = culoare ?? Culori.accent;
    return Semantics(
      label: eticheta,
      toggled: pornit,
      child: GestureDetector(
        onTap: () {
          atingere();
          onSchimba(!pornit);
        },
        child: AnimatedContainer(
          duration: const Duration(milliseconds: 240),
          curve: usor,
          width: 56,
          height: 32,
          padding: const EdgeInsets.all(3),
          decoration: BoxDecoration(
            color: pornit
                ? c
                : (Culori.intunecat
                      ? const Color(0xFF3A3A3D)
                      : const Color(0xFFE4E4E8)),
            borderRadius: BorderRadius.circular(99),
          ),
          child: AnimatedAlign(
            duration: const Duration(milliseconds: 240),
            curve: usor,
            alignment: pornit ? Alignment.centerRight : Alignment.centerLeft,
            child: Container(
              width: 26,
              height: 26,
              decoration: const BoxDecoration(
                color: Colors.white,
                shape: BoxShape.circle,
                boxShadow: [
                  BoxShadow(
                    color: Color(0x26000000),
                    blurRadius: 3,
                    offset: Offset(0, 1),
                  ),
                ],
              ),
              alignment: Alignment.center,
              child: asteapta
                  ? SizedBox(
                      width: 12,
                      height: 12,
                      child: CircularProgressIndicator(
                        strokeWidth: 1.6,
                        color: pornit ? c : Culori.text2,
                      ),
                    )
                  : Icon(
                      Icons.power_settings_new_rounded,
                      size: 15,
                      color: pornit ? c : const Color(0xFF9A9AA0),
                    ),
            ),
          ),
        ),
      ),
    );
  }
}

// ---------------------------------------------------------------- bare

// bara aluneca in 0,5 s, cu aceeasi curba ca pe panou. Cu dungi = "casa clasica", de comparat
class Bara extends StatelessWidget {
  final double procent;
  final Color? culoare;
  final Color? fundal;
  final double inaltime;
  final bool dungi;
  const Bara(
    this.procent, {
    super.key,
    this.culoare,
    this.fundal,
    this.inaltime = 8,
    this.dungi = false,
  });

  @override
  Widget build(BuildContext context) {
    final c = culoare ?? Culori.accent;
    return Container(
      height: inaltime,
      decoration: BoxDecoration(
        color: fundal ?? Culori.moale,
        borderRadius: BorderRadius.circular(99),
      ),
      alignment: Alignment.centerLeft,
      child: TweenAnimationBuilder<double>(
        tween: Tween(end: (procent / 100).clamp(0, 1).toDouble()),
        duration: const Duration(milliseconds: 500),
        curve: usor,
        builder: (_, v, _) => FractionallySizedBox(
          widthFactor: v,
          heightFactor: 1,
          child: ClipRRect(
            borderRadius: BorderRadius.circular(99),
            child: dungi
                ? CustomPaint(painter: _Dungi(c))
                : ColoredBox(color: c),
          ),
        ),
      ),
    );
  }
}

class _Dungi extends CustomPainter {
  final Color c;
  _Dungi(this.c);

  @override
  void paint(Canvas canvas, Size s) {
    canvas.drawRect(
      Offset.zero & s,
      Paint()..color = c.withValues(alpha: 0.35),
    );
    final p = Paint()
      ..color = c
      ..strokeWidth = 3;
    for (var x = -s.height; x < s.width; x += 7) {
      canvas.drawLine(Offset(x, s.height), Offset(x + s.height, 0), p);
    }
  }

  @override
  bool shouldRepaint(_Dungi vechi) => vechi.c != c;
}

// bateria in casute: cate una la fiecare 10%
class BaraCasute extends StatelessWidget {
  final int procent;
  final Color culoare;
  final double inaltime;
  final int casute;
  const BaraCasute(
    this.procent, {
    super.key,
    required this.culoare,
    this.inaltime = 34,
    this.casute = 10,
  });

  @override
  Widget build(BuildContext context) {
    final pline = (procent.clamp(0, 100) / 100 * casute).round();
    return SizedBox(
      height: inaltime,
      child: Row(
        children: [
          for (var i = 0; i < casute; i++) ...[
            if (i > 0) const SizedBox(width: 5),
            Expanded(
              child: AnimatedContainer(
                duration: Duration(milliseconds: 200 + 40 * i),
                decoration: BoxDecoration(
                  color: i < pline ? culoare : Culori.moale,
                  borderRadius: BorderRadius.circular(7),
                ),
              ),
            ),
          ],
        ],
      ),
    );
  }
}

// ---------------------------------------------------------------- punct care pulseaza

class Punct extends StatefulWidget {
  final Color culoare;
  final bool pulseaza;
  final double marime;
  const Punct({
    super.key,
    required this.culoare,
    this.pulseaza = false,
    this.marime = 8,
  });

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
    width: widget.marime,
    height: widget.marime,
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
                  opacity: (0.4 * (1 - min(1, t / 0.7))).clamp(0, 1).toDouble(),
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
    final pulseaza =
        c == Conexiune.conectat ||
        c == Conexiune.internet ||
        c == Conexiune.seConecteaza;
    return Sticla(
      padding: const EdgeInsets.symmetric(horizontal: 13, vertical: 9),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Punct(culoare: c.culoare, pulseaza: pulseaza),
          const SizedBox(width: 8),
          T(c.text, marime: 12, gros: FontWeight.w600),
        ],
      ),
    );
  }
}

// ---------------------------------------------------------------- inel (procente)

class Inel extends StatelessWidget {
  final int procent;
  final double marime;
  final Color? culoare;
  final String eticheta;
  const Inel(
    this.procent, {
    super.key,
    this.marime = 110,
    this.culoare,
    this.eticheta = 'umiditate',
  });

  @override
  Widget build(BuildContext context) => SizedBox(
    width: marime,
    height: marime,
    child: TweenAnimationBuilder<double>(
      tween: Tween(end: procent.clamp(0, 100) / 100),
      duration: const Duration(milliseconds: 500),
      curve: usor,
      builder: (_, v, _) => CustomPaint(
        painter: _InelPictor(v, culoare ?? Culori.apa),
        child: Center(
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              T('$procent%', marime: marime / 4.2, gros: FontWeight.w300),
              if (eticheta.isNotEmpty) T.mic(eticheta),
            ],
          ),
        ),
      ),
    ),
  );
}

class _InelPictor extends CustomPainter {
  final double p;
  final Color c;
  _InelPictor(this.p, this.c);

  @override
  void paint(Canvas canvas, Size size) {
    const g = 10.0;
    final r = Rect.fromLTWH(g / 2, g / 2, size.width - g, size.height - g);
    final linie = Paint()
      ..style = PaintingStyle.stroke
      ..strokeCap = StrokeCap.round
      ..strokeWidth = g;
    canvas.drawArc(r, 0, 2 * pi, false, linie..color = Culori.moale);
    if (p > 0) canvas.drawArc(r, -pi / 2, 2 * pi * p, false, linie..color = c);
  }

  @override
  bool shouldRepaint(_InelPictor vechi) => vechi.p != p || vechi.c != c;
}

// ---------------------------------------------------------------- grafic mic (sparkline)

// o linie subtire, cu o umbra plata sub ea si un punct la capat (valoarea de acum)
class Sparkline extends StatelessWidget {
  final List<double?> valori;
  final Color culoare;
  final double inaltime;
  const Sparkline(
    this.valori, {
    super.key,
    required this.culoare,
    this.inaltime = 56,
  });

  @override
  Widget build(BuildContext context) {
    final v = valori.whereType<double>().toList();
    if (v.length < 2) {
      return SizedBox(
        height: inaltime,
        child: const Center(child: T.mic('încă nu sunt destule puncte')),
      );
    }
    return SizedBox(
      height: inaltime,
      width: double.infinity,
      child: CustomPaint(painter: _SparkPictor(valori, culoare)),
    );
  }
}

class _SparkPictor extends CustomPainter {
  final List<double?> v;
  final Color c;
  _SparkPictor(this.v, this.c);

  @override
  void paint(Canvas canvas, Size s) {
    final bune = v.whereType<double>();
    var mic = bune.reduce(min), mare = bune.reduce(max);
    if (mare - mic < 1e-6) {
      mic -= 1;
      mare += 1;
    }
    const sus = 6.0, jos = 4.0;
    Offset punct(int i, double y) => Offset(
      i / (v.length - 1) * s.width,
      sus + (1 - (y - mic) / (mare - mic)) * (s.height - sus - jos),
    );

    final linie = Path(), plin = Path();
    Offset? ultimul;
    var incepe = true;
    for (var i = 0; i < v.length; i++) {
      final y = v[i];
      if (y == null) {
        incepe = true; // gol in date: rupem linia
        continue;
      }
      final p = punct(i, y);
      if (incepe) {
        linie.moveTo(p.dx, p.dy);
        plin.moveTo(p.dx, s.height);
        plin.lineTo(p.dx, p.dy);
        incepe = false;
      } else {
        linie.lineTo(p.dx, p.dy);
        plin.lineTo(p.dx, p.dy);
      }
      ultimul = p;
    }
    if (ultimul != null) plin.lineTo(ultimul.dx, s.height);
    plin.close();

    canvas.drawPath(plin, Paint()..color = c.withValues(alpha: 0.12));
    canvas.drawPath(
      linie,
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1.8
        ..strokeJoin = StrokeJoin.round
        ..color = c,
    );
    if (ultimul != null) {
      canvas.drawCircle(ultimul, 5, Paint()..color = Culori.card);
      canvas.drawCircle(ultimul, 3.5, Paint()..color = c);
    }
  }

  @override
  bool shouldRepaint(_SparkPictor vechi) => vechi.v != v || vechi.c != c;
}

// ---------------------------------------------------------------- rand de "date tehnice"

// eticheta in stanga, valoarea scrisa ca un cod in dreapta (in ferestrele "Date avansate")
class RandTehnic extends StatelessWidget {
  final String eticheta, valoare;
  final Color? culoare;
  const RandTehnic(this.eticheta, this.valoare, {super.key, this.culoare});

  @override
  Widget build(BuildContext context) => Padding(
    padding: const EdgeInsets.symmetric(vertical: 6),
    child: Row(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Expanded(child: T.mic(eticheta)),
        const SizedBox(width: 12),
        Flexible(
          flex: 2,
          child: T.cod(
            valoare,
            marime: 12,
            culoare: culoare ?? Culori.text,
            aliniere: TextAlign.right,
          ),
        ),
      ],
    ),
  );
}
