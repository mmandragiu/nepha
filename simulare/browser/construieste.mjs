// Construieste simulare/casa-virtuala.html: programul lui Arduino si al panoului,
// compilate pentru browser cu Emscripten (https://emscripten.org).
//
//   node simulare/browser/construieste.mjs
//
// Trebuie sa aveti Emscripten instalat si activat (em++ in PATH, sau EMSDK setat).
// Pagina rezultata e un singur fisier: se deschide cu dublu-click, fara server.

import { cpSync, copyFileSync, mkdirSync, rmSync, readdirSync, existsSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const aici = dirname(fileURLToPath(import.meta.url));
const radacina = join(aici, "..", "..");

// 1. o copie a panoului, cu ecranul inlocuit de cel simulat (codul panoului ramane neatins)
const copie = join(aici, "construit", "panou");
rmSync(join(aici, "construit"), { recursive: true, force: true });
mkdirSync(copie, { recursive: true });
for (const f of readdirSync(join(radacina, "panou"))) {
  if (f.endsWith(".h") || f.endsWith(".ino")) cpSync(join(radacina, "panou", f), join(copie, f));
}
copyFileSync(join(aici, "ecran.h"), join(copie, "ecran.h"));

// 2. compilarea
const emsdk = process.env.EMSDK;
const empp = process.platform === "win32"
  ? (emsdk ? join(emsdk, "upstream", "emscripten", "em++.bat") : "em++.bat")
  : (emsdk ? join(emsdk, "upstream", "emscripten", "em++") : "em++");
if (emsdk && !existsSync(empp)) throw new Error("Nu gasesc em++ in " + empp);

const iesire = join(radacina, "simulare", "casa-virtuala.html");
const argumente = [
  "sim.cpp", "-o", iesire,
  "-O2", "-std=gnu++17", "-I", ".",
  "-Wno-deprecated-declarations", "-Wno-unused-value",
  "--shell-file", "shell.html",
  "-sSINGLE_FILE=1", "-sALLOW_MEMORY_GROWTH=1", "-sENVIRONMENT=web",
  "-sEXPORTED_FUNCTIONS=_malloc,_free",
  "-sEXPORTED_RUNTIME_METHODS=UTF8ToString,stringToNewUTF8,HEAPU8",
];
execFileSync(empp, argumente, { cwd: aici, stdio: "inherit", shell: process.platform === "win32" });
rmSync(join(aici, "construit"), { recursive: true, force: true });
console.log("gata: " + iesire);
