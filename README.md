# Physik-W-Seminar-2026-27
# ⚖️ DualLoadCell — Arduino UNO WiFi R2 + HX711 Dashboard

Ein vollständiges Messsystem für zwei HX711-verstärkte Wägezellen (je 1 kg) auf Basis des **Arduino UNO WiFi R2**. Der Arduino stellt die Messdaten über ein lokales WLAN als HTTP-API bereit; ein browserbasiertes Dashboard visualisiert die Werte in Echtzeit, ermöglicht die Kalibrierung und erlaubt den Export der Messdaten.

-----

## 📋 Inhaltsverzeichnis

- [Funktionen](#funktionen)
- [Hardware](#hardware)
- [Verdrahtung](#verdrahtung)
- [Voraussetzungen](#voraussetzungen)
- [Installation & Quickstart](#installation--quickstart)
- [HTTP-API Referenz](#http-api-referenz)
- [Dashboard-Bedienung](#dashboard-bedienung)
- [Kalibrierungs-Assistent](#kalibrierungs-assistent)
- [Dateistruktur](#dateistruktur)
- [Lizenz](#lizenz)

-----

## Funktionen

- **Dual-Kanal-Messung** — zwei unabhängige HX711-Module werden gleichzeitig ausgelesen
- **WLAN-Streaming** — Messwerte werden über einen integrierten HTTP-Server als JSON bereitgestellt (Port 80, Polling alle 500 ms)
- **Persistente Kalibrierung** — Kalibrierfaktoren werden im EEPROM gespeichert und überleben einen Neustart
- **Geführter Kalibrierungs-Assistent** — Schritt-für-Schritt-Prozess direkt im Browser (Tare → Referenzgewicht auflegen → Faktor berechnen)
- **Live-Diagramm** — interaktiver Chart mit einstellbarem Zeitfenster (1 / 5 / 10 Minuten oder alle Daten)
- **Messtabelle** — tabellarische Darstellung aller Messwerte mit Zeitstempel, Einzelwerten und Summe
- **Datenexport** — CSV, JSON und Chart-Bild (PNG) mit einem Klick
- **Min / Max / Durchschnitt** — Statistiken werden je Kanal live berechnet

-----

## Hardware

|Komponente          |Anzahl|Hinweis                            |
|--------------------|------|-----------------------------------|
|Arduino UNO WiFi R2 |1     |                                   |
|HX711 Breakout-Modul|2     |z. B. SparkFun, Adafruit oder Clone|
|Wägezelle 1 kg      |2     |Wheatstone-Brücke, 4-Draht         |
|Jumperkabel         |—     |                                   |

-----

## Verdrahtung

```
HX711 Modul #1  →  Arduino
  VCC           →  5V
  GND           →  GND
  DT            →  Pin 5
  SCK           →  Pin 4

HX711 Modul #2  →  Arduino
  VCC           →  5V
  GND           →  GND
  DT            →  Pin 9
  SCK           →  Pin 8
```

Die Wägezellen werden nach dem Standard-Schema an die HX711-Module angeschlossen (E+, E−, A+, A−).

-----

## Voraussetzungen

### Arduino-Bibliotheken

Im **Arduino Library Manager** installieren:

- `HX711` by **Bogdan Necula** (Version ≥ 0.7)
- `WiFiNINA` (vorinstalliert für UNO WiFi R2, sonst über Library Manager)

### Dashboard

Nur ein moderner Browser — kein Build-Tool, kein Server, keine Installation.

-----

## Installation & Quickstart

### 1. Sketch konfigurieren

`DualLoadCell.ino` öffnen und WLAN-Zugangsdaten eintragen:

```cpp
const char* WIFI_SSID     = "DEIN_WLAN_NAME";
const char* WIFI_PASSWORD = "DEIN_WLAN_PASSWORT";
```

### 2. Sketch hochladen

Sketch über die Arduino IDE auf den UNO WiFi R2 hochladen.

### 3. IP-Adresse notieren

Seriellen Monitor öffnen (115200 Baud). Nach erfolgreichem WLAN-Verbindungsaufbau erscheint:

```
Arduino IP: 192.168.1.XXX
Webserver läuft auf Port 80.
```

### 4. Dashboard öffnen

`loadcell_dashboard.html` direkt im Browser öffnen (kein Webserver nötig).

### 5. Verbinden

Die IP-Adresse aus Schritt 3 in das IP-Feld des Dashboards eingeben und **VERBINDEN** klicken. Der grüne Statusindikator bestätigt die erfolgreiche Verbindung.

-----

## HTTP-API Referenz

Alle Endpunkte antworten mit JSON. CORS ist für alle Origins aktiviert.

|Methode|Endpunkt                        |Beschreibung                                                          |
|-------|--------------------------------|----------------------------------------------------------------------|
|`GET`  |`/data`                         |Aktuelle Messwerte beider Zellen als JSON                             |
|`GET`  |`/tare?cell=1|2`                |Tare (Nullstellung) für die gewählte Zelle durchführen                |
|`GET`  |`/calibrate/start?cell=1|2`     |Kalibrierung starten (führt Tare durch, wartet auf Referenzgewicht)   |
|`GET`  |`/calibrate/setweight?grams=<g>`|Bekanntes Referenzgewicht in Gramm übergeben, Kalibrierung abschließen|
|`GET`  |`/calibrate/cancel`             |Laufende Kalibrierung abbrechen                                       |
|`GET`  |`/reset?cell=1|2`               |Kalibrierfaktor der Zelle auf Standardwert zurücksetzen               |

### Beispiel-Antwort `/data`

```json
{
  "cell1": 123.45,
  "cell2": 98.76,
  "unit": "g",
  "cal1": -7050.00,
  "cal2": -7050.00,
  "calibMode": false,
  "calibStep": 0,
  "calibTarget": 1,
  "ts": 42301
}
```

-----

## Dashboard-Bedienung

|Element                |Funktion                                                   |
|-----------------------|-----------------------------------------------------------|
|**IP-Feld + VERBINDEN**|Verbindung zum Arduino herstellen oder trennen             |
|**TARE ZELLE 1 / 2**   |Zelle nullstellen (Wiegeplatte muss leer sein)             |
|**KALIBRIERUNG**       |Kalibrierungs-Assistent öffnen                             |
|**Zeitfenster-Auswahl**|Sichtbaren Zeitraum im Diagramm einstellen                 |
|**DATEN LÖSCHEN**      |Alle aufgezeichneten Messwerte und Statistiken zurücksetzen|
|**EXPORT CSV**         |Alle Messungen als Semikolon-getrennte CSV-Datei speichern |
|**EXPORT JSON**        |Alle Messungen als strukturierte JSON-Datei speichern      |
|**EXPORT CHART PNG**   |Aktuellen Graphen als PNG-Bild speichern                   |

-----

## Kalibrierungs-Assistent

1. **KALIBRIERUNG** im Dashboard anklicken
1. Zelle auswählen (Zelle 1 oder Zelle 2)
1. Wiegeplatte **leer lassen** — der Arduino führt automatisch eine Tare durch
1. Ein **Gewicht mit bekannter Masse** (z. B. 500 g) auflegen
1. Die Grammzahl in das Eingabefeld tippen und **KALIBRIEREN** klicken
1. Der neue Kalibrierfaktor wird berechnet und dauerhaft im EEPROM des Arduino gespeichert

> **Tipp:** Für beste Ergebnisse ein Kalibriergewicht verwenden, das möglichst nah am tatsächlichen Messbereich liegt.

-----

## Dateistruktur

```
DualLoadCell/
├── DualLoadCell.ino          # Arduino-Sketch (Firmware)
└── loadcell_dashboard.html   # Web-Dashboard (Browser)
```

Detaillierte Beschreibungen der Dateien: siehe [`DualLoadCell.ino`](#duaLoading-Zelleino) und [`loadcell_dashboard.html`](#loadcell_dashboardhtml) unten.

-----

##  Datei-Beschreibungen

### `DualLoadCell.ino`

**Arduino-Firmware für den UNO WiFi R2.**

Initialisiert beim Start beide HX711-Module auf den konfigurierten Pins, lädt gespeicherte Kalibrierfaktoren aus dem EEPROM und führt eine automatische Nullstellung (Tare) durch. Danach verbindet sich der Arduino mit dem konfigurierten WLAN-Netzwerk und startet einen einfachen HTTP-Server auf Port 80.

Im Hauptloop werden die Messwerte alle 200 ms aktualisiert. Eingehende HTTP-Anfragen werden synchron bearbeitet: Der Sketch parst die Request-Line, routet auf den passenden Handler und antwortet mit JSON. CORS-Header sind gesetzt, sodass das Dashboard aus dem Browser direkt auf die API zugreifen kann.

Der Kalibrierungsablauf ist zustandsbasiert: `/calibrate/start` führt eine Tare durch und wechselt in den Kalibrierungsmodus. Sobald `/calibrate/setweight` mit der bekannten Grammzahl aufgerufen wird, berechnet der Sketch den neuen Kalibrierfaktor und schreibt ihn über `EEPROM.put()` persistent in den Flash-Speicher.

**Wichtige Konstanten:**

|Konstante         |Standardwert|Bedeutung                                |
|------------------|------------|-----------------------------------------|
|`MEASURE_INTERVAL`|200 ms      |Messintervall                            |
|`calibFactor1/2`  |-7050.0     |Kalibrierfaktor (Standardwert)           |
|`MAX_ROWS`        |(Dashboard) |Maximale Anzahl an gepufferten Messwerten|

-----

### `loadcell_dashboard.html`

**Browserbasiertes Echtzeit-Dashboard — vollständig in einer einzigen HTML-Datei.**

Enthält HTML, CSS und JavaScript ohne externe Abhängigkeiten außer Chart.js (wird über CDN geladen). Die Datei kann direkt mit dem Browser geöffnet werden — kein lokaler Webserver, kein Build-Schritt notwendig.

**Aufbau:**

- **Verbindungsleiste (Header):** IP-Eingabe, Verbindungsschalter, Kalibrierungsbutton und ein farbiger Statusindikator (grün = verbunden, rot = Fehler)
- **Gauge-Karten:** Zeigen den aktuellen Messwert jeder Zelle als große Zahl, einen Fortschrittsbalken sowie Min/Max/Durchschnitt-Statistiken
- **Live-Chart:** Zeitreihen-Liniendiagramm mit Chart.js; das sichtbare Zeitfenster ist frei wählbar; Messwerte werden ohne Animation aktualisiert, um Flackern zu vermeiden
- **Messtabelle:** Scrollbare Tabelle mit Zeitstempel, Einzelwerten beider Zellen und der Summe; neue Einträge erscheinen oben
- **Export-Funktionen:** CSV (Semikolon-getrennt, ISO-8601-Zeitstempel), JSON (strukturiertes Objekt mit Metadaten) und PNG-Diagrammexport über die Chart.js-Canvas-API
- **Kalibrierungs-Assistent:** Modales Overlay mit dreistufigem Workflow; kommuniziert direkt mit den `/calibrate/*`-Endpunkten des Arduino
- **Statuszeile (Footer):** Zeigt die letzte Log-Nachricht sowie die aktuellen Kalibrierfaktoren beider Zellen

Das Dashboard pollt den `/data`-Endpunkt alle 500 ms. Bei Verbindungsfehlern (Timeout > 1,5 s) wechselt der Statusindikator auf Rot und die Fehlermeldung erscheint in der Statuszeile. Polling und Verbindung können jederzeit über den Verbinden/Trennen-Schalter gesteuert werden.

-----

## Lizenz

MIT License — frei verwendbar und anpassbar.
