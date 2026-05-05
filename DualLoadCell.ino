/*
 * DualLoadCell.ino
 * Arduino UNO WiFi R2 - Dual HX711 Load Cell Reader
 *
 * Verkabelung:
 *   HX711 #1 (Zelle 1): DT -> Pin 5, SCK -> Pin 4
 *   HX711 #2 (Zelle 2): DT -> Pin 9, SCK -> Pin 8
 *
 * Bibliotheken (Arduino Library Manager):
 *   - "HX711" by Bogdan Necula
 *   - "WiFiNINA" (vorinstalliert fuer UNO WiFi R2)
 */

#include <HX711.h>
#include <WiFiNINA.h>
#include <EEPROM.h>

// ---- Pin-Definitionen -------------------------------------------------------
#define CELL1_DT   5
#define CELL1_SCK  4
#define CELL2_DT   9
#define CELL2_SCK  8

// ---- WLAN-Zugangsdaten  (HIER ANPASSEN!) ------------------------------------
const char* WIFI_SSID     = "DEIN_WLAN_NAME";
const char* WIFI_PASSWORD = "DEIN_WLAN_PASSWORT";

// ---- HTTP-Server auf Port 80 ------------------------------------------------
WiFiServer server(80);

// ---- HX711-Objekte ----------------------------------------------------------
HX711 scale1;
HX711 scale2;

// ---- EEPROM-Adressen fuer persistente Kalibrierwerte ------------------------
#define EEPROM_ADDR_CAL1  0   // float  4 Bytes
#define EEPROM_ADDR_CAL2  4   // float  4 Bytes
#define EEPROM_VALID_FLAG 8   // byte   0xAB = Werte gueltig

float calibFactor1 = -7050.0;
float calibFactor2 = -7050.0;

// ---- Kalibrierungs-Zustand --------------------------------------------------
bool  calibMode   = false;
int   calibStep   = 0;   // 1 = tare fertig, warte auf Gewicht
int   calibTarget = 1;
float knownWeight = 500.0;

// ---- Messwert-Puffer --------------------------------------------------------
float weight1 = 0.0;
float weight2 = 0.0;
unsigned long lastMeasure = 0;
const unsigned long MEASURE_INTERVAL = 200; // ms

// =============================================================================
// EEPROM
// =============================================================================
void loadCalibration() {
  if (EEPROM.read(EEPROM_VALID_FLAG) == 0xAB) {
    EEPROM.get(EEPROM_ADDR_CAL1, calibFactor1);
    EEPROM.get(EEPROM_ADDR_CAL2, calibFactor2);
    Serial.println(F("[EEPROM] Kalibrierwerte geladen."));
  } else {
    Serial.println(F("[EEPROM] Keine gespeicherten Werte - Standard aktiv."));
  }
}

void saveCalibration() {
  EEPROM.put(EEPROM_ADDR_CAL1, calibFactor1);
  EEPROM.put(EEPROM_ADDR_CAL2, calibFactor2);
  EEPROM.write(EEPROM_VALID_FLAG, 0xAB);
  Serial.println(F("[EEPROM] Gespeichert."));
}

void applyCalibration() {
  scale1.set_scale(calibFactor1);
  scale2.set_scale(calibFactor2);
}

// =============================================================================
// Kalibrierung
// =============================================================================
void doTare(int cell) {
  Serial.print(F("[CAL] Tare Zelle "));
  Serial.println(cell);
  if (cell == 1) scale1.tare(10);
  else           scale2.tare(10);
  Serial.println(F("[CAL] Tare fertig."));
}

void computeCalibration(int cell, float grams) {
  // Rohwert (ohne Skalierung) = get_units * aktueller Faktor
  if (cell == 1) {
    float raw = scale1.get_units(10) * calibFactor1;
    calibFactor1 = raw / grams;
    scale1.set_scale(calibFactor1);
    Serial.print(F("[CAL] Faktor Zelle 1: "));
    Serial.println(calibFactor1);
  } else {
    float raw = scale2.get_units(10) * calibFactor2;
    calibFactor2 = raw / grams;
    scale2.set_scale(calibFactor2);
    Serial.print(F("[CAL] Faktor Zelle 2: "));
    Serial.println(calibFactor2);
  }
  saveCalibration();
}

// =============================================================================
// HTTP-Hilfsfunktionen
// =============================================================================
void sendCORS(WiFiClient& c) {
  c.println(F("Access-Control-Allow-Origin: *"));
  c.println(F("Access-Control-Allow-Methods: GET, OPTIONS"));
  c.println(F("Access-Control-Allow-Headers: Content-Type"));
}

void sendJSON(WiFiClient& c, String body) {
  c.println(F("HTTP/1.1 200 OK"));
  c.println(F("Content-Type: application/json"));
  sendCORS(c);
  c.println(F("Connection: close"));
  c.println();
  c.print(body);
}

void sendOK(WiFiClient& c, String msg) {
  sendJSON(c, "{\"status\":\"ok\",\"message\":\"" + msg + "\"}");
}

void sendError(WiFiClient& c, String msg) {
  c.println(F("HTTP/1.1 400 Bad Request"));
  c.println(F("Content-Type: application/json"));
  sendCORS(c);
  c.println(F("Connection: close"));
  c.println();
  c.print("{\"status\":\"error\",\"message\":\"" + msg + "\"}");
}

String getParam(const String& url, const String& key) {
  int idx = url.indexOf(key + "=");
  if (idx == -1) return "";
  int start = idx + key.length() + 1;
  int end   = url.indexOf('&', start);
  if (end == -1) end = url.indexOf(' ', start);
  if (end == -1) end = url.length();
  return url.substring(start, end);
}

// =============================================================================
// HTTP-Request-Dispatcher
// =============================================================================
void handleClient(WiFiClient& client) {
  // Erste Zeile lesen (Request-Line)
  String req = "";
  while (client.available()) {
    char ch = client.read();
    if (ch == '\n') break;
    if (ch != '\r') req += ch;
  }
  // Restliche Header verwerfen
  while (client.available()) {
    String line = client.readStringUntil('\n');
    if (line == "\r" || line.length() == 0) break;
  }

  Serial.print(F("[HTTP] "));
  Serial.println(req);

  // /data  - Messwerte als JSON
  if (req.startsWith("GET /data")) {
    String j = "{";
    j += "\"cell1\":"  + String(weight1, 2)    + ",";
    j += "\"cell2\":"  + String(weight2, 2)    + ",";
    j += "\"unit\":\"g\",";
    j += "\"cal1\":"   + String(calibFactor1, 2) + ",";
    j += "\"cal2\":"   + String(calibFactor2, 2) + ",";
    j += "\"calibMode\":" + String(calibMode ? "true" : "false") + ",";
    j += "\"calibStep\":" + String(calibStep) + ",";
    j += "\"calibTarget\":" + String(calibTarget) + ",";
    j += "\"ts\":"     + String(millis());
    j += "}";
    sendJSON(client, j);
  }

  // /tare?cell=1|2
  else if (req.startsWith("GET /tare")) {
    int cell = getParam(req, "cell").toInt();
    if (cell == 1 || cell == 2) {
      doTare(cell);
      sendOK(client, "Tare Zelle " + String(cell) + " abgeschlossen");
    } else {
      sendError(client, "cell muss 1 oder 2 sein");
    }
  }

  // /calibrate/start?cell=1|2
  else if (req.startsWith("GET /calibrate/start")) {
    calibTarget = getParam(req, "cell").toInt();
    if (calibTarget != 1 && calibTarget != 2) calibTarget = 1;
    calibMode = true;
    calibStep = 1;
    doTare(calibTarget);
    sendOK(client, "Kalibrierung Zelle " + String(calibTarget) + " gestartet. Bekanntes Gewicht auflegen, dann /calibrate/setweight aufrufen.");
  }

  // /calibrate/setweight?grams=500
  else if (req.startsWith("GET /calibrate/setweight")) {
    knownWeight = getParam(req, "grams").toFloat();
    if (knownWeight <= 0) {
      sendError(client, "Ungueltige Grammangabe");
    } else if (!calibMode || calibStep != 1) {
      sendError(client, "Nicht im Kalibrierungsmodus. /calibrate/start zuerst aufrufen.");
    } else {
      computeCalibration(calibTarget, knownWeight);
      calibMode = false;
      calibStep = 0;
      sendOK(client, "Kalibrierung abgeschlossen. Faktor gespeichert.");
    }
  }

  // /calibrate/cancel
  else if (req.startsWith("GET /calibrate/cancel")) {
    calibMode = false;
    calibStep = 0;
    applyCalibration();
    sendOK(client, "Kalibrierung abgebrochen");
  }

  // /reset?cell=1|2
  else if (req.startsWith("GET /reset")) {
    int cell = getParam(req, "cell").toInt();
    if (cell == 1) { calibFactor1 = -7050.0; scale1.set_scale(calibFactor1); }
    else           { calibFactor2 = -7050.0; scale2.set_scale(calibFactor2); }
    saveCalibration();
    sendOK(client, "Zelle " + String(cell) + " auf Standardwert zurueckgesetzt");
  }

  // OPTIONS (CORS Pre-flight)
  else if (req.startsWith("OPTIONS")) {
    client.println(F("HTTP/1.1 204 No Content"));
    sendCORS(client);
    client.println(F("Connection: close"));
    client.println();
  }

  // 404
  else {
    client.println(F("HTTP/1.1 404 Not Found"));
    client.println(F("Content-Type: text/plain"));
    sendCORS(client);
    client.println(F("Connection: close"));
    client.println();
    client.println(F("Verfuegbare Endpoints:"));
    client.println(F("  GET /data"));
    client.println(F("  GET /tare?cell=1|2"));
    client.println(F("  GET /calibrate/start?cell=1|2"));
    client.println(F("  GET /calibrate/setweight?grams=<g>"));
    client.println(F("  GET /calibrate/cancel"));
    client.println(F("  GET /reset?cell=1|2"));
  }
}

// =============================================================================
// SETUP
// =============================================================================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);

  Serial.println(F("\n=== DualLoadCell v1.0 ==="));
  Serial.println(F("Zelle 1: DT=5  SCK=4"));
  Serial.println(F("Zelle 2: DT=9  SCK=8"));

  // HX711 starten
  scale1.begin(CELL1_DT, CELL1_SCK);
  scale2.begin(CELL2_DT, CELL2_SCK);

  // Kalibrierung aus EEPROM
  loadCalibration();
  applyCalibration();

  // Start-Tare
  Serial.println(F("Tare beider Zellen (Wiegeplatte leer lassen)..."));
  scale1.tare(10);
  scale2.tare(10);
  Serial.println(F("Tare abgeschlossen."));

  // WLAN
  Serial.print(F("Verbinde mit WLAN '"));
  Serial.print(WIFI_SSID);
  Serial.print(F("'"));
  int tries = 0;
  while (WiFi.begin(WIFI_SSID, WIFI_PASSWORD) != WL_CONNECTED && tries < 20) {
    delay(1000);
    Serial.print('.');
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F(" verbunden!"));
    Serial.print(F("Arduino IP: "));
    Serial.println(WiFi.localIP());
    server.begin();
    Serial.println(F("Webserver laeuft auf Port 80."));
    Serial.println(F("Trage diese IP-Adresse im Dashboard ein."));
  } else {
    Serial.println(F("\n[FEHLER] Keine WLAN-Verbindung! SSID/Passwort pruefen."));
  }
}

// =============================================================================
// LOOP
// =============================================================================
void loop() {
  // Messwerte aktualisieren
  if (millis() - lastMeasure >= MEASURE_INTERVAL) {
    lastMeasure = millis();
    if (scale1.is_ready()) weight1 = scale1.get_units(1);
    if (scale2.is_ready()) weight2 = scale2.get_units(1);
  }

  // HTTP-Anfragen beantworten
  WiFiClient client = server.available();
  if (client) {
    handleClient(client);
    client.flush();
    delay(2);
    client.stop();
  }
}
