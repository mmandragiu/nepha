// istoric_local - graficul pe 3 zile, tinut pe telefon (SQLite)
//
// Panoul tine doar ultimele 24 de ore (un punct la 5 minute). Ori de cate ori aplicatia ia
// graficul de la panou, scriem aici punctele pe care nu le avem inca (cele vechi nu se mai
// rescriu) si stergem ce e mai vechi de 3 zile. Asa, chiar daca aplicatia a stat inchisa,
// golurile se umplu la urmatoarea deschidere, cat timp n-au trecut mai mult de 24 de ore.
//
// Un rand = un punct: t (minutul, de la 1970), temperaturile, bateria, puterile.
// 3 zile x 288 de puncte = ~860 de randuri, sub 50 KB.
import 'package:flutter/foundation.dart';
import 'package:path/path.dart' as p;
import 'package:sqflite/sqflite.dart';

import '../models/api_models.dart';

const _zile = 3;

class IstoricLocal {
  Database? _db;

  // SQLite e doar pe telefon (Android / iPhone); pe calculator sau in browser nu tinem nimic
  bool get merge =>
      !kIsWeb &&
      (defaultTargetPlatform == TargetPlatform.android ||
          defaultTargetPlatform == TargetPlatform.iOS);

  Future<Database?> _deschide() async {
    if (!merge) return null;
    return _db ??= await openDatabase(
      p.join(await getDatabasesPath(), 'istoric.db'),
      version: 1,
      onCreate: (db, _) => db.execute(
        'CREATE TABLE puncte (t INTEGER PRIMARY KEY, t_casa REAL, t_afara REAL, '
        'baterie INTEGER, solar REAL, vant REAL, consum REAL)',
      ),
    );
  }

  // `minutAcum` = ora casei de acum (minutul zilei), ca sa stim din ce zi e fiecare punct
  Future<void> adauga(Istoric i, int minutAcum) async {
    final db = await _deschide();
    if (db == null || i.minut.isEmpty) return;
    final acum = DateTime.now();
    final miezulNoptii = DateTime(acum.year, acum.month, acum.day);
    final m0 = miezulNoptii.millisecondsSinceEpoch ~/ 60000;
    final b = db.batch();
    for (var k = 0; k < i.minut.length; k++) {
      // panoul tine 24 de ore: un minut "de mai tarziu" decat acum e de ieri
      var t = m0 + i.minut[k];
      if (i.minut[k] > minutAcum + 5) t -= 24 * 60;
      b.rawInsert('INSERT OR IGNORE INTO puncte VALUES (?, ?, ?, ?, ?, ?, ?)', [
        t,
        k < i.tCasa.length ? i.tCasa[k] : null,
        k < i.tAfara.length ? i.tAfara[k] : null,
        k < i.baterie.length ? i.baterie[k] : 0,
        k < i.solar.length ? i.solar[k] : 0,
        k < i.vant.length ? i.vant[k] : 0,
        k < i.consum.length ? i.consum[k] : 0,
      ]);
    }
    final limita = acum.millisecondsSinceEpoch ~/ 60000 - _zile * 24 * 60;
    b.delete('puncte', where: 't < ?', whereArgs: [limita]);
    await b.commit(noResult: true);
  }

  // ultimele 3 zile; in `minut` e minutul de la 1970 (nu minutul zilei)
  Future<Istoric?> citeste() async {
    final db = await _deschide();
    if (db == null) return null;
    final r = await db.query('puncte', orderBy: 't');
    if (r.isEmpty) return null;
    double nr(Object? x) => (x as num?)?.toDouble() ?? 0;
    return Istoric(
      minut: [for (final x in r) x['t'] as int],
      tCasa: [for (final x in r) (x['t_casa'] as num?)?.toDouble()],
      tAfara: [for (final x in r) (x['t_afara'] as num?)?.toDouble()],
      baterie: [for (final x in r) (x['baterie'] as int?) ?? 0],
      solar: [for (final x in r) nr(x['solar'])],
      vant: [for (final x in r) nr(x['vant'])],
      consum: [for (final x in r) nr(x['consum'])],
    );
  }
}
