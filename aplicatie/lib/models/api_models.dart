// modelele - exact ce trimite panoul (vezi panou/web.ino si api-panou.md).
// Unde Arduino nu stie o valoare (o camera fara termometru), JSON-ul are null.

double? _nr(Object? v) => v is num ? v.toDouble() : null;
double _nr0(Object? v) => v is num ? v.toDouble() : 0;
int _int(Object? v) => v is num ? v.toInt() : 0;
bool _bool(Object? v) => v == true;
String _text(Object? v) => v is String ? v : '';
Map<String, dynamic> _obiect(Object? v) =>
    v is Map<String, dynamic> ? v : const {};
List<dynamic> _lista(Object? v) => v is List ? v : const [];

class Camera {
  final String nume;
  final double? temp; // grade; null = fara termometru
  final int lumina; // 0-100
  final bool prezenta; // e cineva in camera
  final bool fereastra; // geam deschis
  final int? umiditate; // % aer, doar in camera cu DHT11
  final bool livolo; // lumina e pe un intrerupator Livolo

  const Camera({
    required this.nume,
    this.temp,
    required this.lumina,
    required this.prezenta,
    required this.fereastra,
    this.umiditate,
    this.livolo = false,
  });

  factory Camera.fromJson(Map<String, dynamic> j) => Camera(
    nume: _text(j['nume']),
    temp: _nr(j['temp']),
    lumina: _int(j['lumina']),
    prezenta: _bool(j['prezenta']),
    fereastra: _bool(j['fereastra']),
    umiditate: j['umiditate'] is num ? _int(j['umiditate']) : null,
    livolo: _bool(j['livolo']),
  );
}

class Incalzire {
  final bool pornita;
  final double tinta;
  final String motiv; // text gata scris
  final int
  motivCod; // 0 clasic, 1 e cineva, 2 preincalzire, 3 plecati, 4 nu e nimeni, 5 noapte, 6 fereastra
  final double rata; // grade pe ora, invatat
  final double? confort; // temperatura de confort tinuta de Arduino

  const Incalzire({
    required this.pornita,
    required this.tinta,
    required this.motiv,
    required this.motivCod,
    required this.rata,
    this.confort,
  });

  factory Incalzire.fromJson(Map<String, dynamic> j) => Incalzire(
    pornita: _bool(j['pornita']),
    tinta: _nr0(j['tinta']),
    motiv: _text(j['motiv']),
    motivCod: _int(j['motivCod']),
    rata: _nr0(j['rata']),
    confort: _nr(j['confort']),
  );
}

class Energie {
  final int baterie; // %
  final double solar, vant, consum; // W, la scara machetei (valori mici)
  final String mod; // Normal, Surplus, Economie, Critic
  final int modCod; // 0..3 in aceeasi ordine
  final double whAzi; // consumul casei NEPHA azi (socotit pentru o casa reala)
  final double whClasic; // cat ar fi consumat o casa clasica
  final double mwh; // masurat pe macheta
  final double? estimatSolar6h; // Wh din soare in urmatoarele 6 ore
  final double factorPanou;

  const Energie({
    required this.baterie,
    required this.solar,
    required this.vant,
    required this.consum,
    required this.mod,
    required this.modCod,
    required this.whAzi,
    required this.whClasic,
    required this.mwh,
    this.estimatSolar6h,
    required this.factorPanou,
  });

  factory Energie.fromJson(Map<String, dynamic> j) => Energie(
    baterie: _int(j['baterie']),
    solar: _nr0(j['solar']),
    vant: _nr0(j['vant']),
    consum: _nr0(j['consum']),
    mod: _text(j['mod']),
    modCod: _int(j['modCod']),
    whAzi: _nr0(j['whAzi']),
    whClasic: _nr0(j['whClasic']),
    mwh: _nr0(j['mwh']),
    estimatSolar6h: _nr(j['estimatSolar6h']),
    factorPanou: _nr0(j['factorPanou']),
  );

  // kWh economisiti azi fata de o casa clasica
  double get kwhEconomisiti =>
      ((whClasic - whAzi) / 1000).clamp(0, double.infinity).toDouble();
}

class SenzorUsa {
  final String nume;
  final bool deschis;

  const SenzorUsa({required this.nume, required this.deschis});

  factory SenzorUsa.fromJson(Map<String, dynamic> j) =>
      SenzorUsa(nume: _text(j['nume']), deschis: _bool(j['deschis']));
}

class Alarma {
  final String stare; // text gata scris
  final int
  cod; // 0 dezarmata, 1 se armeaza, 2 armata noapte, 3 armata plecat, 4 cere PIN, 5 suna
  final List<SenzorUsa> senzori;

  const Alarma({required this.stare, required this.cod, required this.senzori});

  factory Alarma.fromJson(Map<String, dynamic> j) => Alarma(
    stare: _text(j['stare']),
    cod: _int(j['cod']),
    senzori: _lista(
      j['senzori'],
    ).map((s) => SenzorUsa.fromJson(_obiect(s))).toList(),
  );

  bool get pericol => cod >= 4;
}

class Gradina {
  final int umiditate; // %
  final bool pompa;
  // de la placa din gradina; lipsesc daca nu e pornita
  final bool? ploua;
  final int zona; // ce zona se uda acum (0 = niciuna)

  const Gradina({
    required this.umiditate,
    required this.pompa,
    this.ploua,
    this.zona = 0,
  });

  factory Gradina.fromJson(Map<String, dynamic> j) => Gradina(
    umiditate: _int(j['umiditate']),
    pompa: _bool(j['pompa']),
    ploua: j['ploua'] is bool ? j['ploua'] as bool : null,
    zona: _int(j['zona']),
  );
}

// prognoza; daca nu e valida, restul campurilor lipsesc
class Vreme {
  final bool valida;
  final double? temp;
  final int? ploaie, soare, nori, vant;
  final int? oraPloaie; // lipseste daca nu vine ploaia

  const Vreme({
    required this.valida,
    this.temp,
    this.ploaie,
    this.soare,
    this.nori,
    this.vant,
    this.oraPloaie,
  });

  factory Vreme.fromJson(Map<String, dynamic> j) => Vreme(
    valida: _bool(j['valida']),
    temp: _nr(j['temp']),
    ploaie: j['ploaie'] is num ? _int(j['ploaie']) : null,
    soare: j['soare'] is num ? _int(j['soare']) : null,
    nori: j['nori'] is num ? _int(j['nori']) : null,
    vant: j['vant'] is num ? _int(j['vant']) : null,
    oraPloaie: j['oraPloaie'] is num ? _int(j['oraPloaie']) : null,
  );
}

// ziua de test; valorile sunt in Wh. Daca nu e valid, celelalte campuri lipsesc
class RezultatTest {
  final bool valid;
  final double lumini, incalzire, pompa, priza, total, mwh, litri;

  const RezultatTest({
    required this.valid,
    this.lumini = 0,
    this.incalzire = 0,
    this.pompa = 0,
    this.priza = 0,
    this.total = 0,
    this.mwh = 0,
    this.litri = 0,
  });

  factory RezultatTest.fromJson(Map<String, dynamic> j) => RezultatTest(
    valid: _bool(j['valid']),
    lumini: _nr0(j['lumini']),
    incalzire: _nr0(j['incalzire']),
    pompa: _nr0(j['pompa']),
    priza: _nr0(j['priza']),
    total: _nr0(j['total']),
    mwh: _nr0(j['mwh']),
    litri: _nr0(j['litri']),
  );
}

class Retea {
  final String ip, nume;
  final bool ap; // e pe reteaua proprie NEPHA-casa
  final bool
  internet; // panoul vorbeste cu brokerul (merge si de pe date mobile)

  const Retea({
    required this.ip,
    required this.nume,
    required this.ap,
    this.internet = false,
  });

  factory Retea.fromJson(Map<String, dynamic> j) => Retea(
    ip: _text(j['ip']),
    nume: _text(j['nume']),
    ap: _bool(j['ap']),
    internet: _bool(j['internet']),
  );
}

// GET /api/stare
class Stare {
  final String
  server; // numele unic al panoului (adresa lui MAC, ex. a0b1c2d3e4f5)
  final bool primit; // panoul primeste date de la Arduino?
  final int vechime; // de cate secunde n-au mai venit date (-1 = niciodata)
  final String ora;
  final int minut; // minutul zilei
  final String mod; // "smart" sau "clasic"
  final int test; // 0 nu, 1 pe macheta, 2 calcul rapid
  final List<Camera> camere;
  final double? afara;
  final int luminaNaturala;
  final Incalzire incalzire;
  final Energie energie;
  final Alarma alarma;
  final Gradina gradina;
  final bool priza;
  final int senzori;
  final Vreme vreme;
  final List<RezultatTest> rezultate; // [0] casa clasica, [1] casa NEPHA
  final Retea retea;
  final double pretKwh;

  const Stare({
    this.server = '',
    required this.primit,
    required this.vechime,
    required this.ora,
    required this.minut,
    required this.mod,
    required this.test,
    required this.camere,
    this.afara,
    required this.luminaNaturala,
    required this.incalzire,
    required this.energie,
    required this.alarma,
    required this.gradina,
    required this.priza,
    required this.senzori,
    required this.vreme,
    required this.rezultate,
    required this.retea,
    required this.pretKwh,
  });

  factory Stare.fromJson(Map<String, dynamic> j) => Stare(
    server: _text(j['server']),
    primit: _bool(j['primit']),
    vechime: j['vechime'] is num ? _int(j['vechime']) : -1,
    ora: _text(j['ora']),
    minut: _int(j['minut']),
    mod: _text(j['mod']),
    test: _int(j['test']),
    camere: _lista(
      j['camere'],
    ).map((c) => Camera.fromJson(_obiect(c))).toList(),
    afara: _nr(j['afara']),
    luminaNaturala: _int(j['luminaNaturala']),
    incalzire: Incalzire.fromJson(_obiect(j['incalzire'])),
    energie: Energie.fromJson(_obiect(j['energie'])),
    alarma: Alarma.fromJson(_obiect(j['alarma'])),
    gradina: Gradina.fromJson(_obiect(j['gradina'])),
    priza: _bool(j['priza']),
    senzori: _int(j['senzori']),
    vreme: Vreme.fromJson(_obiect(j['vreme'])),
    rezultate: _lista(
      j['rezultate'],
    ).map((r) => RezultatTest.fromJson(_obiect(r))).toList(),
    retea: Retea.fromJson(_obiect(j['retea'])),
    pretKwh: _nr0(j['pretKwh']),
  );

  bool get smart => mod == 'smart';

  // Arduino vorbeste cu panoul (a trimis ceva in ultimele 10 secunde)
  bool get arduinoVorbeste => primit && vechime >= 0 && vechime < 10;

  // media camerelor care au termometru
  double? get tempCasa {
    final t = camere.map((c) => c.temp).whereType<double>().toList();
    return t.isEmpty ? null : t.reduce((a, b) => a + b) / t.length;
  }
}

// GET /api/jurnal
class Decizie {
  final String ora;
  final int? cod;
  final String text;

  const Decizie({required this.ora, this.cod, required this.text});

  factory Decizie.fromJson(Map<String, dynamic> j) => Decizie(
    ora: _text(j['ora']),
    cod: j['cod'] is num ? _int(j['cod']) : null,
    text: _text(j['text']),
  );
}

// GET /api/istoric - puncte la 5 minute
class Istoric {
  final List<int> minut;
  final List<double> solar, vant, consum;
  final List<int> baterie;
  final List<double?> tCasa, tAfara;

  const Istoric({
    required this.minut,
    required this.solar,
    required this.vant,
    required this.consum,
    required this.baterie,
    required this.tCasa,
    required this.tAfara,
  });

  factory Istoric.fromJson(Map<String, dynamic> j) => Istoric(
    minut: _lista(j['minut']).map(_int).toList(),
    solar: _lista(j['solar']).map(_nr0).toList(),
    vant: _lista(j['vant']).map(_nr0).toList(),
    consum: _lista(j['consum']).map(_nr0).toList(),
    baterie: _lista(j['baterie']).map(_int).toList(),
    tCasa: _lista(j['tCasa']).map(_nr).toList(),
    tAfara: _lista(j['tAfara']).map(_nr).toList(),
  );
}

// GET /api/rutina
class Rutina {
  final bool valida;
  final List<String> camere;
  final List<List<int>> p; // [camera][ora 0-23] = % sanse sa fie cineva
  final List<int> zile;

  const Rutina({
    required this.valida,
    required this.camere,
    required this.p,
    required this.zile,
  });

  factory Rutina.fromJson(Map<String, dynamic> j) => Rutina(
    valida: _bool(j['valida']),
    camere: _lista(j['camere']).map(_text).toList(),
    p: _lista(j['p']).map((r) => _lista(r).map(_int).toList()).toList(),
    zile: _lista(j['zile']).map(_int).toList(),
  );
}
