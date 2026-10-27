// ui_jurnal - tot ce a hotarat casa, cu ora si motivul. Cele mai noi sus.

uint8_t paginaJurnal = 0;
#define PE_PAGINA 4

void desenJurnal(bool tot) {
  uint8_t pagini = max(1, (nrJurnal + PE_PAGINA - 1) / PE_PAGINA);
  if (paginaJurnal >= pagini) paginaJurnal = pagini - 1;
  String semn = String(nrJurnal) + (nrJurnal ? jurnal[0].minut : 0) + paginaJurnal;
  if (!schimbat(0, semn)) return;

  card(12, 50, 456, 216);
  if (!nrJurnal) {
    paragraf("Nicio decizie încă.", 28, 66, 420, F_MIC, C_TEXT2, C_CARD, 1);
    return;
  }
  for (uint8_t k = 0; k < PE_PAGINA; k++) {
    uint8_t i = paginaJurnal * PE_PAGINA + k;
    if (i >= nrJurnal) break;
    int y = 60 + k * 51;
    if (k) tft.drawFastHLine(28, y - 6, 380, C_GRI_D);
    scrie(oraText(jurnal[i].minut), 28, y, F_MIC, C_VERDE, C_CARD);
    paragraf(textEveniment(jurnal[i]), 80, y, 330, F_MIC, C_TEXT, C_CARD, 2);
  }
  scrie(String(paginaJurnal + 1) + "/" + String(pagini), 440, 158, F_MIC, C_TEXT2, C_CARD, textdatum_t::middle_center);
  buton("‹", 420, 60, 40, 40, 90);
  buton("›", 420, 216, 40, 40, 91);
}

void atingeJurnal(uint8_t id) {
  if (id == 90 && paginaJurnal > 0) paginaJurnal--;
  if (id == 91) paginaJurnal++;
  dateNoiUI = true;
}
