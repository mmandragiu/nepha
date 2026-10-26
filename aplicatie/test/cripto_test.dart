// PIN-ul criptat pentru panou: acelasi format pe care il desface panou/imperechere.ino
// (iv 12 octeti | text criptat | eticheta 16 octeti, in base64; numele panoului intra in verificare)
import 'dart:convert';

import 'package:cryptography/cryptography.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:nepha/services/broker.dart';

void main() {
  test(
    'comanda cu PIN se poate desface doar cu cheia si numele panoului',
    () async {
      final cheie = List.generate(32, (i) => i);
      final secret = await cripteazaComanda(
        cheie,
        'nepha-a1b2c3',
        'ALARMA OPRIT 1234',
      );
      final b = base64.decode(secret);
      expect(b.length, 12 + '1793012345 ALARMA OPRIT 1234'.length + 16);

      final aes = AesGcm.with256bits();
      Future<String> desface(String server) async => utf8.decode(
        await aes.decrypt(
          SecretBox(
            b.sublist(12, b.length - 16),
            nonce: b.sublist(0, 12),
            mac: Mac(b.sublist(b.length - 16)),
          ),
          secretKey: SecretKey(cheie),
          aad: utf8.encode(server),
        ),
      );
      final text = await desface('nepha-a1b2c3');
      expect(text, endsWith(' ALARMA OPRIT 1234'));
      final trimis = int.parse(text.split(' ').first);
      expect(
        (DateTime.now().millisecondsSinceEpoch ~/ 1000 - trimis).abs(),
        lessThan(5),
      );
      // alt panou (alt nume) nu poate desface mesajul
      await expectLater(
        desface('nepha-000000'),
        throwsA(isA<SecretBoxAuthenticationError>()),
      );
    },
  );
}
