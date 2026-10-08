// Pune pagina web (web/index.html) in firmware-ul panoului.
// O comprima (gzip) si o scrie ca un sir de octeti in panou/web_pagina.h.
//
//   node unelte/web_in_panou.mjs
//
// Dupa asta se urca din nou programul pe panou.

import { readFileSync, writeFileSync } from "node:fs";
import { gzipSync } from "node:zlib";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const radacina = join(dirname(fileURLToPath(import.meta.url)), "..");
const html = readFileSync(join(radacina, "web", "index.html"));
const gz = gzipSync(html, { level: 9 });

const randuri = [];
for (let i = 0; i < gz.length; i += 20) {
  randuri.push("  " + Array.from(gz.subarray(i, i + 20), (b) => "0x" + b.toString(16).padStart(2, "0")).join(", "));
}

const iesire =
  "// Generat din web/index.html cu unelte/web_in_panou.mjs - nu se editeaza de mana.\n" +
  `// ${html.length} octeti de HTML, ${gz.length} dupa comprimare.\n` +
  "#pragma once\n#include <Arduino.h>\n\n" +
  "const uint8_t PAGINA_GZ[] PROGMEM = {\n" +
  randuri.join(",\n") +
  "\n};\n";

writeFileSync(join(radacina, "panou", "web_pagina.h"), iesire);
console.log(`web_pagina.h: ${html.length} -> ${gz.length} octeti`);
