"""
Face fonturile pentru ecranul panoului, in formatul VLW ("smooth font", cel
folosit de LovyanGFX / TFT_eSPI), cu diacriticele romanesti.

Fonturile care vin cu bibliotecile au doar litere englezesti, asa ca pe ecran
"Încălzire" iesea cu patratele. Scriptul asta ia fontul Nunito (licenta OFL,
gratuit) si deseneaza fiecare litera in marimea de care avem nevoie.

Folosire:
    pip install pillow fonttools brotli
    python fonturi_vlw.py <folder cu nunito-*.woff> ../panou/fonturi.h

Fisierele .woff le gasiti in pachetul @fontsource/nunito (npm) sau pe
fonts.google.com (atunci dati calea catre .ttf, merge la fel).
"""

import os
import struct
import sys

from fontTools.ttLib import TTFont
from PIL import Image, ImageDraw, ImageFont

ROMANESTI = "ăâîșțĂÂÎȘȚşţŞŢ"
SIMBOLURI = "°·–—−„”“…•‹›"
TOT = "".join(chr(c) for c in range(32, 127)) + ROMANESTI + SIMBOLURI
CIFRE = " 0123456789.,:-+%°CWhkVlm/"

# (nume in cod, grosime, marime in pixeli, ce caractere)
FONTURI = [
    ("font_mic", 700, 15, TOT),
    ("font_text", 700, 19, TOT),
    ("font_titlu", 800, 26, TOT),
    ("font_mare", 800, 52, CIFRE),
]


def deschide(folder, grosime, temp):
    """Intoarce o lista de (cale ttf, caractere suportate) pentru grosimea data."""
    rezultat = []
    for subset in ("latin", "latin-ext"):
        for ext in (".ttf", ".woff2", ".woff"):
            cale = os.path.join(folder, f"nunito-{subset}-{grosime}-normal{ext}")
            if os.path.exists(cale):
                break
        else:
            continue
        f = TTFont(cale)
        if f.flavor:  # woff/woff2 -> ttf, ca sa-l poata citi Pillow
            f.flavor = None
            cale = os.path.join(temp, f"nunito-{subset}-{grosime}.ttf")
            f.save(cale)
        rezultat.append((cale, set(f.getBestCmap().keys())))
    return rezultat


def litera(font, ch):
    stanga, sus, dreapta, jos = font.getbbox(ch, anchor="ls")
    latime, inaltime = max(0, dreapta - stanga), max(0, jos - sus)
    avans = int(round(font.getlength(ch)))
    imagine = Image.new("L", (max(1, latime), max(1, inaltime)), 0)
    if latime and inaltime:
        ImageDraw.Draw(imagine).text((-stanga, -sus), ch, font=font, fill=255, anchor="ls")
    pixeli = imagine.tobytes() if latime and inaltime else b""
    # VLW: inaltime, latime, avans, cat urca deasupra liniei de baza, decalaj stanga
    return latime, inaltime, avans, -sus, stanga, pixeli


def vlw(surse, marime, caractere):
    fonturi = [(ImageFont.truetype(cale, marime), cmap) for cale, cmap in surse]
    urcare, coborare = fonturi[0][0].getmetrics()
    glife = []
    for ch in sorted(set(caractere), key=ord):
        for font, cmap in fonturi:
            if ord(ch) in cmap:
                glife.append((ord(ch),) + litera(font, ch))
                break
        else:
            print(f"  lipseste U+{ord(ch):04X}")
    antet = struct.pack(">6i", len(glife), 11, urcare + coborare, 0, urcare, coborare)
    metrici = b"".join(struct.pack(">7i", u, h, w, a, dy, dx, 0) for u, w, h, a, dy, dx, _ in glife)
    pixeli = b"".join(g[6] for g in glife)
    return antet + metrici + pixeli, len(glife)


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(1)
    folder, iesire = sys.argv[1], sys.argv[2]
    temp = os.path.join(os.path.dirname(os.path.abspath(iesire)), ".fonturi_temp")
    os.makedirs(temp, exist_ok=True)

    bucati = [
        "// Generat cu unelte/fonturi_vlw.py din fontul Nunito (SIL Open Font License 1.1,\n"
        "// vezi panou/LICENTA-Nunito.txt). Nu se editeaza de mana.\n",
        "#pragma once\n#include <Arduino.h>\n",
    ]
    for nume, grosime, marime, caractere in FONTURI:
        date, n = vlw(deschide(folder, grosime, temp), marime, caractere)
        print(f"{nume}: {n} litere, {len(date)} octeti")
        randuri = [", ".join(str(b) for b in date[i:i + 24]) for i in range(0, len(date), 24)]
        bucati.append(f"\n// Nunito {grosime}, {marime} px\n"
                      f"const uint8_t {nume}[] PROGMEM = {{\n  " + ",\n  ".join(randuri) + "\n};\n")

    with open(iesire, "w", encoding="utf-8", newline="\n") as f:
        f.write("".join(bucati))
    for fisier in os.listdir(temp):
        os.remove(os.path.join(temp, fisier))
    os.rmdir(temp)


if __name__ == "__main__":
    main()
