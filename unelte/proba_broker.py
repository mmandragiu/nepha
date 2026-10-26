# proba_broker.py - incercam aplicatia si brokerul MQTT fara panoul ESP32
#
#   python unelte/proba_broker.py panou     un panou de proba ("nepha-proba") pe laptop, cu o
#                                           casa inventata. Raspunde si pe Wi-Fi (la adresa
#                                           laptopului, portul 8080), si prin broker (nepha-server).
#                                           Arata un cod de legare si desface PIN-ul criptat.
#   python unelte/proba_broker.py telefon [nume-panou]
#                                           se da drept telefon (nepha-client): intreaba panoul
#                                           de stare prin broker si arata ce primeste
#
# Parolele le ia din fisierele ignorate de git (nu le scrie nicaieri si nu le afiseaza):
#   panou/secrete.h: cheile de inrolare (INROLARE_PANOURI, INROLARE_TELEFOANE, INROLARE_RESET).
#   Cu ele isi face cont pe broker, ca panoul adevarat (vezi panou/inrolare.ino).
# Trebuie: pip install paho-mqtt cryptography
#
# Topicele sunt aceleasi ca in panou/config.h si aplicatie/lib/services/broker.dart;
# daca le schimbati acolo, schimbati-le si aici.
import base64
import json
import os
import random
import re
import socket
import ssl
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs

import urllib.error
import urllib.request

import paho.mqtt.client as mqtt

BROKER, PORT = "mqtt.mandragiu.eu", 8883
TOPIC_CERERI = "nepha/servers/{server}/requests"
TOPIC_RASPUNS = "nepha/servers/{server}/clients/{client}/responses"
TOPIC_NOUTATI = "nepha/servers/{server}/events/devices"
AUTH = "https://nepha-auth.mandragiu.eu"
SERVER = "nephaproba01"         # numele "panoului" de proba (cel adevarat foloseste adresa MAC)
PORT_WIFI = 8080
RADACINA = Path(__file__).resolve().parent.parent

casa = {"alarma": 0, "lumini": [80, 0, 45]}   # ce se schimba din aplicatie
cheie = os.urandom(32)                         # cheia casei de proba
cod_legare = random.randint(100000, 999999)


def topic(model, server="", client=""):
    return model.replace("{server}", server).replace("{client}", client)


def chei():
    text = (RADACINA / "panou" / "secrete.h").read_text(encoding="utf-8")
    gaseste = lambda nume: re.search(r'^\s*#define\s+' + nume + r'\s+"([^"]*)"', text, re.M).group(1)
    return gaseste("INROLARE_PANOURI"), gaseste("INROLARE_TELEFOANE"), gaseste("INROLARE_RESET")


# ca panou/inrolare.ino: cont nou; daca exista deja, parola noua (reset)
def inroleaza(server_id, client_id=None):
    k_panouri, k_telefoane, k_reset = chei()
    corp = {"serverId": server_id}
    if client_id:
        corp["clientId"] = client_id
    cale = "/enroll/client" if client_id else "/enroll/server"
    for cheie, reset in ((k_telefoane if client_id else k_panouri, False), (k_reset, True)):
        if reset:
            corp["reset"] = True
        cerere = urllib.request.Request(AUTH + cale, data=json.dumps(corp).encode(), method="POST",
                                        headers={"Content-Type": "application/json", "X-Enroll-Key": cheie})
        try:
            with urllib.request.urlopen(cerere, timeout=10) as raspuns:
                j = json.loads(raspuns.read())
                print(f"inrolare {cale}{' (parola noua)' if reset else ''}: cont {j['username']}")
                return j["username"], j["password"]
        except urllib.error.HTTPError as e:
            print(f"inrolare {cale}{' (parola noua)' if reset else ''}: {e.code}")
    raise SystemExit("serverul de conturi n-a dat cont")


def client_nou(id_client, utilizator, parola):
    c = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=id_client)
    c.username_pw_set(utilizator, parola)
    c.tls_set(cert_reqs=ssl.CERT_REQUIRED)
    return c


def adresa_laptop():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("8.8.8.8", 80))     # nu trimite nimic, doar afla pe ce placa de retea iesim
        return s.getsockname()[0]
    finally:
        s.close()


def stare_inventata():
    acum = time.localtime()
    return {
        "server": SERVER, "primit": True, "vechime": 1, "ora": time.strftime("%H:%M"),
        "minut": acum.tm_hour * 60 + acum.tm_min, "mod": "smart", "test": 0,
        "camere": [
            {"nume": "Sufragerie", "temp": round(21 + random.random(), 1), "lumina": casa["lumini"][0], "prezenta": True, "fereastra": False},
            {"nume": "Dormitor", "temp": 19.4, "lumina": casa["lumini"][1], "prezenta": False, "fereastra": False},
            {"nume": "Bucătărie", "temp": 21.1, "lumina": casa["lumini"][2], "prezenta": False, "fereastra": False},
        ],
        "afara": 7.5, "luminaNaturala": 12,
        "incalzire": {"pornita": True, "tinta": 21.5, "motiv": "e cineva acasă", "motivCod": 1, "rata": 3.4},
        "energie": {"baterie": 78, "solar": 0.1, "vant": 0.3, "consum": 1.2, "mod": "normal", "modCod": 0,
                    "whAzi": 5200, "whClasic": 7400, "mwh": 0},
        "alarma": {"stare": ["oprită", "se armează", "acasă (noapte)", "plecat"][casa["alarma"]], "cod": casa["alarma"],
                   "senzori": [{"nume": "Ușa de la intrare", "deschis": False}]},
        "gradina": {"umiditate": 31, "pompa": False}, "priza": True, "senzori": 159,
        "vreme": {"valida": False}, "rezultate": [], "retea": {"ip": adresa_laptop(), "nume": "proba", "ap": False, "internet": True},
        "pretKwh": 1.4,
    }


def date_pentru(ce):
    return {"stare": stare_inventata(), "jurnal": [{"ora": "18:52", "cod": 7, "text": "Proba: am stins lumina"}],
            "rutina": {"valida": False, "camere": [], "p": [], "zile": []},
            "istoric": {"minut": [], "tCasa": [], "tAfara": [], "baterie": [], "solar": [], "vant": [], "consum": []},
            }.get(ce)


# ca panou/imperechere.ino: base64(iv 12 | criptat | eticheta 16), numele panoului in verificare
def desface(secret):
    from cryptography.hazmat.primitives.ciphers.aead import AESGCM
    b = base64.b64decode(secret)
    text = AESGCM(cheie).decrypt(b[:12], b[12:], SERVER.encode()).decode()
    trimis, comanda = text.split(" ", 1)
    if abs(time.time() - int(trimis)) > 120:
        raise ValueError("mesaj prea vechi")
    return comanda


def executa(comanda, de_unde):
    print(f"[{de_unde}] comanda: {comanda if not comanda.startswith('ALARMA OPRIT') else 'ALARMA OPRIT ****'}")
    parti = comanda.split()
    if parti[:2] == ["ALARMA", "OPRIT"]:
        casa["alarma"] = 0 if parti[2:] == ["1234"] else casa["alarma"]
        print(f"[{de_unde}] PIN {'bun' if casa['alarma'] == 0 else 'gresit'} (pe panoul de proba PIN-ul e 1234)")
    elif parti[:1] == ["ALARMA"]:
        casa["alarma"] = 2 if "ACASA" in parti else 3
    elif parti[:1] == ["LUMINA"] and len(parti) == 3 and parti[2].isdigit():
        casa["lumini"][int(parti[1]) % 3] = int(parti[2])


class Wifi(BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def trimite(self, cod, j):
        b = json.dumps(j).encode()
        self.send_response(cod)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(b)))
        self.end_headers()
        self.wfile.write(b)

    def do_GET(self):
        date = date_pentru(self.path.removeprefix("/api/"))
        self.trimite(200 if date is not None else 404, date or {})

    def do_POST(self):
        global cod_legare
        f = parse_qs(self.rfile.read(int(self.headers.get("Content-Length", 0))).decode())
        if self.path == "/api/comanda":
            executa(f.get("c", [""])[0].upper(), "Wi-Fi")
            self.trimite(200, {"ok": True})
        elif self.path == "/api/imperechere":
            if f.get("cod", [""])[0] == str(cod_legare):
                print("[Wi-Fi] telefon legat; cod nou:", cod_legare := random.randint(100000, 999999))
                raspuns = {"ok": True, "server": SERVER, "cheie": base64.b64encode(cheie).decode()}
                try:   # ca panoul: ii facem telefonului cont pe broker
                    raspuns["utilizator"], raspuns["parola"] = inroleaza(SERVER, f.get("client", [""])[0])
                except (OSError, AttributeError, SystemExit) as e:
                    print("[Wi-Fi] fara cont pe broker pentru telefon:", e)
                self.trimite(200, raspuns)
            else:
                self.trimite(403, {"ok": False})
        else:
            self.trimite(404, {})


def panou():
    threading.Thread(target=ThreadingHTTPServer(("", PORT_WIFI), Wifi).serve_forever, daemon=True).start()
    print(f"Wi-Fi: in aplicatie scrieti adresa {adresa_laptop()}:{PORT_WIFI} (telefonul in aceeasi retea)")
    print(f"codul de legare: {cod_legare}   PIN-ul alarmei: 1234")

    try:
        cont = inroleaza(SERVER)
    except (OSError, AttributeError):
        print("broker: fara cheile de inrolare in panou/secrete.h, merg doar pe Wi-Fi (Ctrl+C ca sa opresti)")
        while True:
            time.sleep(10)
    c = client_nou(SERVER, *cont)
    c.will_set(topic(TOPIC_NOUTATI, SERVER), json.dumps({"tip": "offline", "server": SERVER}))

    def la_conectare(c, u, f, rc, p):
        print("broker: conectat ca panou" if rc == 0 else f"broker: a refuzat ({rc})")
        c.subscribe(topic(TOPIC_CERERI, SERVER))
        c.publish(topic(TOPIC_NOUTATI, SERVER), json.dumps({"tip": "online", "server": SERVER}))

    def la_mesaj(c, u, m):
        cerere = json.loads(m.payload)
        ce, client = cerere.get("cere"), cerere.get("client", "")
        print(f"[broker] {client}: {ce}")
        raspuns = {"id": cerere.get("id"), "ok": True}
        if ce == "comanda":
            try:
                comanda = desface(cerere["secret"]) if "secret" in cerere else cerere.get("c", "")
                if comanda.startswith("ALARMA OPRIT") and "secret" not in cerere:
                    raise ValueError("PIN necriptat")
                if "secret" in cerere:
                    print("[broker] comanda criptata, desfacuta cu cheia casei")
                executa(comanda.upper(), "broker")
            except Exception as e:
                print("[broker] comanda refuzata:", e)
                raspuns = {"id": cerere.get("id"), "ok": False, "eroare": "secret"}
        elif date_pentru(ce) is not None:
            raspuns["date"] = date_pentru(ce)
        c.publish(topic(TOPIC_RASPUNS, SERVER, client), json.dumps(raspuns))

    c.on_connect, c.on_message = la_conectare, la_mesaj
    c.connect(BROKER, PORT)
    c.loop_start()
    print("(Ctrl+C ca sa opresti)")
    while True:
        time.sleep(10)
        c.publish(topic(TOPIC_NOUTATI, SERVER), json.dumps({"tip": "stare", "server": SERVER, "date": stare_inventata()}))


def telefon():
    eu = "probatelefon" + str(random.randint(1000, 9999))
    server = sys.argv[2] if len(sys.argv) > 2 else SERVER
    c = client_nou(eu, *inroleaza(server, eu))

    def la_conectare(c, u, f, rc, p):
        print("broker: conectat ca telefon" if rc == 0 else f"broker: a refuzat ({rc})")
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
    ce = sys.argv[1] if len(sys.argv) > 1 else ""
    {"panou": panou, "telefon": telefon}.get(ce, lambda: print("folosire: proba_broker.py panou | telefon [nume-panou]"))()
