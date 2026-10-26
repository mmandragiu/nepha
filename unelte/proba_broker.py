# proba_broker.py - verificam brokerul MQTT fara panou si fara telefon
#
#   python unelte/proba_broker.py panou     se da drept panou (utilizatorul nepha-server):
#                                           raspunde la cereri cu o casa inventata, ca sa
#                                           putem incerca aplicatia prin internet fara ESP32
#   python unelte/proba_broker.py telefon   se da drept telefon (nepha-client): intreaba
#                                           panoul de stare si arata ce primeste
#
# Parolele le ia din fisierele ignorate de git (nu le scrie nicaieri si nu le afiseaza):
#   panou   -> panou/secrete.h        (MQTT_UTILIZATOR, MQTT_PAROLA)
#   telefon -> aplicatie/secrete.json (MQTT_UTILIZATOR, MQTT_PAROLA)
# Trebuie: pip install paho-mqtt
#
# Topicele sunt aceleasi ca in panou/config.h si aplicatie/lib/services/broker.dart;
# daca le schimbati acolo, schimbati-le si aici.
import json
import random
import re
import ssl
import sys
import time
from pathlib import Path

import paho.mqtt.client as mqtt

BROKER, PORT = "mqtt.mandragiu.eu", 8883
TOPIC_CERERI = "nepha/servers/{server}/requests"
TOPIC_RASPUNS = "nepha/servers/{server}/clients/{client}/responses"
TOPIC_NOUTATI = "nepha/servers/{server}/events/devices"
SERVER = "nepha-proba"          # numele "panoului" de proba
RADACINA = Path(__file__).resolve().parent.parent


def topic(model, server="", client=""):
    return model.replace("{server}", server).replace("{client}", client)


def parole_panou():
    text = (RADACINA / "panou" / "secrete.h").read_text(encoding="utf-8")
    gaseste = lambda nume: re.search(r'^\s*#define\s+' + nume + r'\s+"([^"]*)"', text, re.M).group(1)
    return gaseste("MQTT_UTILIZATOR"), gaseste("MQTT_PAROLA")


def parole_telefon():
    j = json.loads((RADACINA / "aplicatie" / "secrete.json").read_text(encoding="utf-8"))
    return j["MQTT_UTILIZATOR"], j["MQTT_PAROLA"]


def client_nou(id_client, utilizator, parola):
    c = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=id_client)
    c.username_pw_set(utilizator, parola)
    c.tls_set(cert_reqs=ssl.CERT_REQUIRED)
    c.on_connect = lambda c, u, f, rc, p: print("conectat la broker" if rc == 0 else f"brokerul a refuzat: {rc}")
    return c


def stare_inventata():
    return {
        "server": SERVER, "primit": True, "vechime": 1, "ora": time.strftime("%H:%M"),
        "minut": time.localtime().tm_hour * 60 + time.localtime().tm_min, "mod": "smart", "test": 0,
        "camere": [
            {"nume": "Sufragerie", "temp": round(21 + random.random(), 1), "lumina": 80, "prezenta": True, "fereastra": False},
            {"nume": "Dormitor", "temp": 19.4, "lumina": 0, "prezenta": False, "fereastra": False},
            {"nume": "Bucătărie", "temp": 21.1, "lumina": 45, "prezenta": False, "fereastra": False},
        ],
        "afara": 7.5, "luminaNaturala": 12,
        "incalzire": {"pornita": True, "tinta": 21.5, "motiv": "e cineva acasă", "motivCod": 1, "rata": 3.4},
        "energie": {"baterie": 78, "solar": 0.1, "vant": 0.3, "consum": 1.2, "mod": "normal", "modCod": 0,
                    "whAzi": 5200, "whClasic": 7400, "mwh": 0},
        "alarma": {"stare": "oprită", "cod": 0, "senzori": [{"nume": "Ușa de la intrare", "deschis": False}]},
        "gradina": {"umiditate": 31, "pompa": False}, "priza": True, "senzori": 159,
        "vreme": {"valida": False}, "rezultate": [], "retea": {"ip": "-", "nume": "proba", "ap": False, "internet": True},
        "pretKwh": 1.4,
    }


def panou():
    c = client_nou(SERVER, *parole_panou())
    c.will_set(topic(TOPIC_NOUTATI, SERVER), json.dumps({"tip": "offline", "server": SERVER}))

    def la_conectare(c, u, f, rc, p):
        print("conectat ca panou" if rc == 0 else f"brokerul a refuzat: {rc}")
        c.subscribe(topic(TOPIC_CERERI, SERVER))
        c.publish(topic(TOPIC_NOUTATI, SERVER), json.dumps({"tip": "online", "server": SERVER}))

    def la_mesaj(c, u, m):
        cerere = json.loads(m.payload)
        print("cerere:", {k: v for k, v in cerere.items() if k != "secret"}, "(cu secret)" if "secret" in cerere else "")
        if cerere.get("server") not in ("", None, SERVER):
            return
        date = {"stare": stare_inventata(), "jurnal": [{"ora": "18:52", "cod": 7, "text": "Proba: am stins lumina"}],
                "rutina": {"valida": False, "camere": [], "p": [], "zile": []},
                "istoric": {"minut": [], "tCasa": [], "tAfara": [], "baterie": [], "solar": [], "vant": [], "consum": []}
                }.get(cerere.get("cere"))
        raspuns = {"id": cerere.get("id"), "ok": True}
        if date is not None:
            raspuns["date"] = date
        c.publish(topic(TOPIC_RASPUNS, SERVER, cerere["client"]), json.dumps(raspuns))

    c.on_connect, c.on_message = la_conectare, la_mesaj
    c.connect(BROKER, PORT)
    c.loop_start()
    print(f"panoul de proba {SERVER} asculta (Ctrl+C ca sa opresti)")
    while True:
        time.sleep(10)
        c.publish(topic(TOPIC_NOUTATI, SERVER), json.dumps({"tip": "stare", "server": SERVER, "date": stare_inventata()}))


def telefon():
    eu = "nepha-app-proba" + str(random.randint(100, 999))
    server = sys.argv[2] if len(sys.argv) > 2 else SERVER
    c = client_nou(eu, *parole_telefon())

    def la_conectare(c, u, f, rc, p):
        print("conectat ca telefon" if rc == 0 else f"brokerul a refuzat: {rc}")
        c.subscribe(topic(TOPIC_RASPUNS, server, eu))
        c.subscribe(topic(TOPIC_NOUTATI, server))
        c.publish(topic(TOPIC_CERERI, server), json.dumps({"id": "1", "client": eu, "server": server, "cere": "stare"}))

    c.on_connect = la_conectare
    c.on_message = lambda c, u, m: print(m.topic, "->", m.payload[:300].decode(errors="replace"))
    c.connect(BROKER, PORT)
    c.loop_start()
    print(f"intreb panoul {server} de stare; astept 20 de secunde...")
    time.sleep(20)


if __name__ == "__main__":
    {"panou": panou, "telefon": telefon}.get(sys.argv[1] if len(sys.argv) > 1 else "", lambda: print(__doc__ or "panou | telefon"))()
