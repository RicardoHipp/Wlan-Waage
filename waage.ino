// Benötigte Bibliotheken einbinden
#include <HX711.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// =========== KONFIGURATION & DATENSTRUKTUR ===========
#define CONFIG_VERSION "v11" // Eine Versionsnummer, um alte Speicherdaten zu erkennen

// Eine Struktur, die alle Konfigurationsdaten bündelt.
struct Config {
  char version[4]; // Speicher für die Versionsnummer
  float calibration_factor;
  long tare_offset;
  char mqtt_server[40];
  int mqtt_port;
  char mqtt_topic[50];
  bool mqtt_enabled;
};

Config config; // Globale Instanz der Konfiguration

// --- HX711 (Wägezelle) ---
const int DOUT_PIN = D2;
const int CLK_PIN = D1;

// --- WLAN ---
const char* ssid = "SSID here";
const char* password = "Password here";

// --- Timing ---
unsigned long previousMeasureMillis = 0;
const long measureInterval = 1000;

unsigned long previousMqttMillis = 0;
const long mqttInterval = 5000;


// =========== GLOBALE OBJEKTE & VARIABLEN ===========

HX711 scale;
WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server(80);

volatile float currentWeight = 0.0;
volatile long rawValue = 0;

// HTML-Code für die erweiterte Weboberfläche
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WLAN-Waage</title>
    <style>
        :root {
            --color-bg: #f7fafc; --color-card-bg: #ffffff; --color-text-primary: #2d3748;
            --color-text-secondary: #718096; --color-accent: #4299e1; --color-accent-light: #ebf8ff;
            --color-accent-border: #bee3f8; --color-purple: #805ad5; --color-purple-light: #faf5ff;
            --color-orange: #ed8936; --color-orange-hover: #dd6b20; --color-green: #48bb78;
            --color-green-hover: #38a169; --color-blue: #3182ce; --color-blue-hover: #2b6cb0;
            --color-red: #e53e3e; --color-red-hover: #c53030;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            background-color: var(--color-bg); margin: 0; display: flex; align-items: center;
            justify-content: center; min-height: 100vh; padding: 2rem 0;
        }
        .card {
            background-color: var(--color-card-bg); padding: 1.5rem; border-radius: 1rem;
            box-shadow: 0 10px 15px -3px rgba(0,0,0,0.1), 0 4px 6px -2px rgba(0,0,0,0.05);
            text-align: center; max-width: 24rem; width: 90%;
        }
        h1 { font-size: 1.5rem; font-weight: 700; color: var(--color-text-primary); margin-top: 0; margin-bottom: 0.5rem; }
        p { color: var(--color-text-secondary); margin-top: 0; }
        .weight-container { background-color: var(--color-accent-light); border: 2px solid var(--color-accent-border); border-radius: 0.5rem; padding: 1.5rem; margin: 1.5rem 0; }
        .weight-display { font-size: 3.75rem; font-weight: 700; color: var(--color-accent); }
        .weight-unit { font-size: 1.5rem; color: var(--color-accent); vertical-align: baseline; }
        details { text-align: left; margin-top: 1rem; }
        summary { cursor: pointer; color: var(--color-accent); font-weight: 600; }
        .box { margin-top: 1rem; padding: 1rem; background-color: #f9fafb; border-radius: 0.5rem; border: 1px solid #e5e7eb; }
        .raw-value-display { font-family: monospace; font-size: 1.125rem; text-align: center; color: var(--color-purple); background-color: var(--color-purple-light); padding: 0.5rem; border-radius: 0.375rem; margin-bottom: 1rem; }
        .button { width: 100%; color: white; font-weight: 700; padding: 0.75rem 1rem; border-radius: 0.5rem; border: none; cursor: pointer; transition: background-color 0.2s; }
        .button-orange { background-color: var(--color-orange); margin-bottom: 1rem; }
        .button-orange:hover { background-color: var(--color-orange-hover); }
        .button-green { background-color: var(--color-green); }
        .button-green:hover { background-color: var(--color-green-hover); }
        .button-blue { background-color: var(--color-blue); }
        .button-blue:hover { background-color: var(--color-blue-hover); }
        .button-red { background-color: var(--color-red); }
        .button-red:hover { background-color: var(--color-red-hover); }
        input[type="number"], input[type="text"] { width: 100%; border: 2px solid #d1d5db; border-radius: 0.5rem; padding: 0.5rem; margin-bottom: 0.75rem; box-sizing: border-box; }
        .status-message { margin-top: 1rem; height: 1.25rem; color: var(--color-text-secondary); font-size: 0.875rem; transition: opacity 0.5s ease-in-out; }
        .label { display: block; text-align: left; font-size: 0.875rem; font-weight: 500; color: var(--color-text-primary); margin-bottom: 0.25rem; }
        .checkbox-container { display: flex; align-items: center; margin-bottom: 1rem; }
        .stored-value { font-size: 0.75rem; color: var(--color-text-secondary); font-family: monospace; }
    </style>
</head>
<body>
    <div class="card">
        <h1>WLAN-Waage</h1>
        <p>Aktuelles Gewicht</p>
        <div class="weight-container">
            <span id="weightValue" class="weight-display">--.--</span>
            <span class="weight-unit">kg</span>
        </div>
        
        <details>
            <summary>Kalibrierung</summary>
            <div class="box">
                <label class="label">Live Rohwert (nach Tara):</label>
                <p id="rawValue" class="raw-value-display">--</p>
                <button onclick="tareScale()" class="button button-orange">Tara setzen (Nullpunkt)</button>
                <label class="label">Kalibriergewicht (in g):</label>
                <input type="number" id="knownWeight" placeholder="Bekanntes Gewicht in g">
                <button onclick="calibrateScale()" class="button button-green">Mit diesem Gewicht kalibrieren</button>
                <hr style="margin: 1rem 0; border-color: #e5e7eb;">
                <p class="label">Gespeicherte Werte:</p>
                <p class="stored-value">Tara-Offset: <span id="tareOffsetValue">--</span></p>
                <p class="stored-value">Kalibrierungsfaktor: <span id="calFactorValue">--</span></p>
            </div>
        </details>

        <details>
            <summary>MQTT-Einstellungen</summary>
            <div class="box">
                <div class="checkbox-container">
                    <input type="checkbox" id="mqtt_enabled" onchange="saveMqttSettings()">
                    <label for="mqtt_enabled" style="margin-left: 0.5rem;">MQTT-Übertragung aktivieren</label>
                </div>
                <label for="mqtt_server" class="label">MQTT Server:</label><input type="text" id="mqtt_server">
                <label for="mqtt_port" class="label">MQTT Port:</label><input type="number" id="mqtt_port">
                <label for="mqtt_topic" class="label">MQTT Topic:</label><input type="text" id="mqtt_topic">
                <button onclick="saveMqttSettings()" class="button button-blue">MQTT-Einstellungen speichern</button>
            </div>
        </details>

        <details>
            <summary>System</summary>
            <div class="box">
                <p class="label">Setzt alle Einstellungen auf Werkseinstellungen zurück und startet den ESP neu.</p>
                <button onclick="factoryReset()" class="button button-red">Werkseinstellungen wiederherstellen</button>
            </div>
        </details>

        <div id="statusMessage" class="status-message"></div>
    </div>

    <script>
        function showStatus(message, duration = 3000) {
            const el = document.getElementById('statusMessage');
            el.innerText = message;
            el.style.opacity = 1;
            setTimeout(() => { el.style.opacity = 0; }, duration);
        }

        function updateMeasurements() {
            fetch('/data').then(r => r.json()).then(data => {
                const weightInKg = parseFloat(data.weight) / 1000.0;
                document.getElementById('weightValue').innerText = weightInKg.toFixed(2);
                document.getElementById('rawValue').innerText = data.raw;
                document.getElementById('tareOffsetValue').innerText = data.tare_offset;
                document.getElementById('calFactorValue').innerText = parseFloat(data.calibration_factor).toFixed(3);
            }).catch(e => console.error('Fehler beim Abrufen der Messwerte:', e));
        }

        function initializePage() {
            fetch('/data').then(r => r.json()).then(data => {
                updateMeasurements();
                document.getElementById('mqtt_server').value = data.mqtt_server;
                document.getElementById('mqtt_port').value = data.mqtt_port;
                document.getElementById('mqtt_topic').value = data.mqtt_topic;
                document.getElementById('mqtt_enabled').checked = data.mqtt_enabled;
                setInterval(updateMeasurements, 2000);
            }).catch(e => console.error('Fehler beim Initialisieren der Daten:', e));
        }

        function tareScale() {
            showStatus('Setze und speichere Tara...');
            fetch('/tara').then(r => r.text()).then(t => showStatus(t));
        }

        function calibrateScale() {
            const weight = document.getElementById('knownWeight').value;
            if (!weight || weight <= 0) { showStatus('Bitte ein gültiges Gewicht eingeben.', 2000); return; }
            showStatus('Kalibriere und speichere...');
            fetch(`/kalibrieren?gewicht=${weight}`).then(r => r.text()).then(t => showStatus(t, 5000));
        }
        
        function saveMqttSettings() {
            const serverVal = document.getElementById('mqtt_server').value;
            const portVal = document.getElementById('mqtt_port').value;
            const topicVal = document.getElementById('mqtt_topic').value;
            const enabledVal = document.getElementById('mqtt_enabled').checked;
            
            showStatus('Speichere MQTT-Einstellungen...');
            fetch(`/savemqtt?server=${encodeURIComponent(serverVal)}&port=${portVal}&topic=${encodeURIComponent(topicVal)}&enabled=${enabledVal}`)
                .then(r => r.text()).then(t => showStatus(t, 5000));
        }

        function factoryReset() {
            if (confirm('Achtung! Alle gespeicherten Kalibrierungs- und MQTT-Daten werden gelöscht. Fortfahren?')) {
                showStatus('Setze auf Werkseinstellungen zurück...');
                fetch('/reset').then(r => r.text()).then(t => showStatus(t));
            }
        }

        document.addEventListener('DOMContentLoaded', initializePage);
    </script>
</body>
</html>
)rawliteral";


// =========== FUNKTIONEN ===========

void saveConfig() {
  EEPROM.begin(sizeof(Config));
  EEPROM.put(0, config);
  delay(200);
  EEPROM.commit();
  EEPROM.end();
  Serial.println("-> Konfiguration im EEPROM gespeichert.");
}

void loadConfig() {
  EEPROM.begin(sizeof(Config));
  EEPROM.get(0, config);
  EEPROM.end();

  config.version[sizeof(config.version) - 1] = '\0';
  if (strcmp(config.version, CONFIG_VERSION) != 0) {
    Serial.println("!!! EEPROM-Daten veraltet oder ungültig. Setze auf Werkseinstellungen. !!!");
    strncpy(config.version, CONFIG_VERSION, sizeof(config.version));
    config.calibration_factor = 1.0; // Start with 1.0 to see raw values initially
    config.tare_offset = 0L;
    strncpy(config.mqtt_server, "192.168.179.143", sizeof(config.mqtt_server));
    config.mqtt_port = 1883;
    strncpy(config.mqtt_topic, "sensor/waage/gewicht", sizeof(config.mqtt_topic));
    config.mqtt_enabled = false;
    saveConfig();
  } else {
    Serial.println("-> Gültige Konfiguration geladen.");
  }
}

void handleRoot() { server.send_P(200, "text/html", HTML_PAGE); }

void handleData() {
  char jsonBuffer[300];
  snprintf(jsonBuffer, sizeof(jsonBuffer), 
    "{\"weight\":%.2f,\"raw\":%ld,\"tare_offset\":%ld,\"calibration_factor\":%.4f,\"mqtt_server\":\"%s\",\"mqtt_port\":%d,\"mqtt_topic\":\"%s\",\"mqtt_enabled\":%s}",
    currentWeight, rawValue, config.tare_offset, config.calibration_factor, config.mqtt_server, config.mqtt_port, config.mqtt_topic, config.mqtt_enabled ? "true" : "false");
  server.send(200, "application/json", jsonBuffer);
}

void handleTare() {
  scale.tare();
  config.tare_offset = scale.get_offset();
  saveConfig();
  server.send(200, "text/plain", "Tara erfolgreich gesetzt und gespeichert!");
}

void handleCalibrate() {
  if (server.hasArg("gewicht")) {
    float knownWeight = server.arg("gewicht").toFloat();
    if (knownWeight > 0) {
      long value_after_tare = scale.get_value(10); 
      config.calibration_factor = (float)value_after_tare / knownWeight;
      scale.set_scale(config.calibration_factor);
      saveConfig();
      server.send(200, "text/plain", "Kalibrierung erfolgreich gespeichert!");
    } else { server.send(400, "text/plain", "Ungültiges Gewicht."); }
  } else { server.send(400, "text/plain", "Fehlender 'gewicht' Parameter."); }
}

void handleSaveMqtt() {
  if (server.hasArg("server")) strncpy(config.mqtt_server, server.arg("server").c_str(), sizeof(config.mqtt_server));
  if (server.hasArg("port")) config.mqtt_port = server.arg("port").toInt();
  if (server.hasArg("topic")) strncpy(config.mqtt_topic, server.arg("topic").c_str(), sizeof(config.mqtt_topic));
  config.mqtt_enabled = (server.arg("enabled") == "true");

  saveConfig();
  
  client.disconnect();
  client.setServer(config.mqtt_server, config.mqtt_port);
  
  server.send(200, "text/plain", "MQTT-Einstellungen gespeichert!");
}

void handleReset() {
  EEPROM.begin(sizeof(Config));
  EEPROM.write(0, 0);
  EEPROM.commit();
  EEPROM.end();
  server.send(200, "text/plain", "Reset erfolgreich! ESP startet neu...");
  delay(1000);
  ESP.restart();
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Verbinde mit WLAN: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWLAN verbunden!");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
  if (!config.mqtt_enabled || client.connected()) {
    return;
  }
  Serial.print("Versuche, MQTT-Verbindung aufzubauen...");
  String clientId = "WaageClient-";
  clientId += String(WiFi.macAddress());
  if (client.connect(clientId.c_str())) {
    Serial.println("verbunden!");
  } else {
    Serial.print("fehlgeschlagen, rc=");
    Serial.print(client.state());
    Serial.println(" - Erneuter Versuch in 5 Sekunden");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- WLAN-Waage mit Web-Konfiguration (v11) startet ---");

  loadConfig();
  setup_wifi();
  client.setServer(config.mqtt_server, config.mqtt_port);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.on("/tara", HTTP_GET, handleTare);
  server.on("/kalibrieren", HTTP_GET, handleCalibrate);
  server.on("/savemqtt", HTTP_GET, handleSaveMqtt);
  server.on("/reset", HTTP_GET, handleReset);
  server.begin();
  Serial.println("Webserver gestartet. Aufruf unter http://" + WiFi.localIP().toString());

  Serial.println("\n--- DIAGNOSE-START ---");
  Serial.println("Initialisiere Waage...");
  scale.begin(DOUT_PIN, CLK_PIN);
  
  // KORREKTUR: Robuster Start mit Warteschleife
  int retries = 0;
  Serial.print("Warte auf HX711... ");
  while (!scale.is_ready() && retries < 10) { // Versuche es für 5 Sekunden
    Serial.print(".");
    delay(500);
    retries++;
  }
  Serial.println();

  if (scale.is_ready()) {
    Serial.println("-> HX711 gefunden und bereit!");
    
    Serial.printf("1. Aus EEPROM geladen -> Kalib.-Faktor: %.3f | Tara-Offset: %ld\n", config.calibration_factor, config.tare_offset);
    
    scale.set_scale(config.calibration_factor);
    scale.set_offset(config.tare_offset);
    
    Serial.printf("2. In Bibliothek gesetzt -> Kalib.-Faktor: %.3f | Tara-Offset: %ld\n", scale.get_scale(), scale.get_offset());
    
    Serial.println("3. Waage konfiguriert und bereit zum Wiegen.");
  } else {
    Serial.println("\n!!! FEHLER: HX711 nach mehreren Versuchen nicht gefunden. Prüfe die Verkabelung! Programm wird angehalten. !!!");
    while(true) { delay(1000); } // Programm anhalten, da eine Funktion nicht möglich ist
  }
  Serial.println("--- DIAGNOSE-ENDE ---\n");
}

void loop() {
  server.handleClient();
  client.loop();

  unsigned long currentMillis = millis();

  if (currentMillis - previousMeasureMillis >= measureInterval) {
    previousMeasureMillis = currentMillis;
    if (scale.is_ready()) {
      float current_scale_factor = scale.get_scale();
      long current_tare_offset = scale.get_offset();
      
      rawValue = scale.get_value(5); 
      currentWeight = (float)rawValue / current_scale_factor;
      
      Serial.printf("Messung: %.2f g | Roh: %ld | Kalib: %.3f | Tara: %ld\n", currentWeight, rawValue, current_scale_factor, current_tare_offset);
    } else {
      Serial.println("HX711 nicht bereit.");
    }
  }

  if (currentMillis - previousMqttMillis >= mqttInterval) {
    previousMqttMillis = currentMillis;
    if (config.mqtt_enabled) {
      if (!client.connected()) {
        reconnect_mqtt();
      }
      if (client.connected()) {
        char weightString[20];
        dtostrf(currentWeight, 1, 2, weightString);
        client.publish(config.mqtt_topic, weightString);
      }
    }
  }
  
  delay(1);
}
