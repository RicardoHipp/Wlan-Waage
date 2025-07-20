# ESP8266 WLAN-Waage mit Weboberfläche

Dies ist ein Projekt für eine präzise, WLAN-fähige Waage auf Basis eines ESP8266-Mikrocontrollers und eines HX711-Wägezellen-Verstärkers. Die Waage ist vollständig über eine intuitive Weboberfläche konfigurierbar, die direkt vom ESP8266 bereitgestellt wird. Alle Einstellungen werden dauerhaft im EEPROM des Chips gespeichert.

Die Messwerte können live im Browser abgelesen und optional per MQTT an einen Smart-Home-Server (wie Home Assistant, ioBroker, etc.) gesendet werden.

---

# ESP8266 Wi-Fi Scale with Web Interface

This is a project for a precise, Wi-Fi enabled scale based on an ESP8266 microcontroller and an HX711 load cell amplifier. The scale is fully configurable via an intuitive web interface hosted directly on the ESP8266. All settings are persistently stored in the chip's EEPROM.

Measurements can be read live in any web browser and can optionally be sent via MQTT to a smart home server (like Home Assistant, ioBroker, etc.).

---

## Features (Deutsch)

* **Live-Gewichtsanzeige:** Zeigt das aktuelle Gewicht in Kilogramm (kg) mit zwei Nachkommastellen an.
* **Vollständige Web-Konfiguration:** Keine Notwendigkeit, den Code für Anpassungen zu ändern. Alles wird im Browser eingestellt.
* **Online-Kalibrierung:** Tara (Nullpunkt) setzen und Kalibrierung mit einem bekannten Gewicht direkt über die Weboberfläche.
* **Anzeige gespeicherter Werte:** Die aktuellen Tara- und Kalibrierungs-Offsets werden zur Kontrolle angezeigt.
* **MQTT-Integration:** Konfigurierbare MQTT-Einstellungen (Server, Port, Topic, Aktivierung) zur Einbindung in IoT-Systeme.
* **Persistenter Speicher:** Alle Einstellungen (Kalibrierung, MQTT) bleiben auch nach einem Stromausfall erhalten.
* **System-Funktionen:** Ein "Werkseinstellungen"-Button ermöglicht das einfache Zurücksetzen der Konfiguration.
* **Robuster Start:** Eine eingebaute Verzögerung und Überprüfung beim Start stellt sicher, dass der HX711-Chip zuverlässig erkannt wird.

## Features (English)

* **Live Weight Display:** Shows the current weight in kilograms (kg) with two decimal places.
* **Complete Web Configuration:** No need to modify the code for adjustments. Everything is configured in the browser.
* **On-the-fly Calibration:** Set tare (zero point) and calibrate with a known weight directly via the web interface.
* **Display of Stored Values:** The current tare and calibration offsets are displayed for verification.
* **MQTT Integration:** Configurable MQTT settings (server, port, topic, enable/disable) for integration into IoT systems.
* **Persistent Storage:** All settings (calibration, MQTT) are retained even after a power loss.
* **System Functions:** A "Factory Reset" button allows for easy resetting of the configuration.
* **Robust Startup:** A built-in delay and check routine ensures the HX711 chip is reliably detected on startup.

---

## Benötigte Hardware / Required Hardware

| Komponente / Component | Beschreibung / Description |
| :--- | :--- |
| ESP8266 | z.B. Wemos D1 Mini oder NodeMCU / e.g., Wemos D1 Mini or NodeMCU |
| HX711 Modul | Wägezellen-Verstärker (meist auf grüner Platine) / Load cell amplifier (usually on a green board) |
| Wägezelle(n) / Load Cell(s) | 1x 4-Draht-Wägezelle ODER 4x 3-Draht-Wägezellen / 1x 4-wire load cell OR 4x 3-wire load cells |
| Kabel / Wires | Jumper-Kabel zur Verbindung / Jumper wires for connection |
| Optional: Kondensatoren | 1x `10µF` & 1x `0.1µF` zur Stabilisierung der Stromversorgung des HX711 / for stabilizing the HX711 power supply |

---

## Verkabelung / Wiring

Verbinde den HX711 mit dem ESP8266 wie folgt:
Connect the HX711 to the ESP8266 as follows:

| HX711 Pin | ESP8266 Pin |
| :--- | :--- |
| VCC | 5V |
| GND | G (GND) |
| DT | D2 |
| SCK | D1 |

### Option A: Verkabelung mit 1 Wägezelle (4-Draht)
Die Wägezelle wird an die vier Schraubklemmen des HX711 angeschlossen (meist farbcodiert):
The load cell is connected to the four screw terminals of the HX711 (usually color-coded):

* Rot / Red -> E+
* Schwarz / Black -> E-
* Weiß / White -> A-
* Grün / Green -> A+

### Option B: Verkabelung mit 4 Wägezellen (3-Draht, z.B. für Personenwaagen)
Bei diesem Aufbau werden vier 3-Draht-Wägezellen (Halbbrücken) zu einer Wheatstone-Vollbrücke zusammengeschaltet. Dies ist die gängige Methode für Personenwaagen.

**1. Wägezellen untereinander verbinden:**
* Verbinde die weißen Kabel der diagonal gegenüberliegenden Zellen miteinander.
* Verbinde die schwarzen Kabel der diagonal gegenüberliegenden Zellen miteinander.

**2. Mit dem HX711 verbinden:**
* Die vier roten Kabel der Wägezellen werden jeweils an einen der vier Anschlüsse (E+, E-, A+, A-) des HX711 angeschlossen. Die genaue Zuordnung ist für die Funktion nicht kritisch, beeinflusst aber das Vorzeichen des Rohwerts. Sollte die Waage bei Belastung negative Werte anzeigen, müssen die Anschlüsse A+ und A- getauscht werden.


---

## Einrichtung / Setup

1.  **Arduino IDE vorbereiten / Prepare Arduino IDE:**
    * Stelle sicher, dass das ESP8266 Board in deinem Boardverwalter installiert ist.
    * Installiere die benötigten Bibliotheken über den Bibliotheksverwalter:
        * `HX711-ADC` von Olav Kallhovd (olkal)
        * `PubSubClient` von Nick O'Leary

2.  **WLAN-Daten eintragen / Enter Wi-Fi Credentials:**
    * Öffne den Code in der Arduino IDE.
    * **WICHTIG:** Trage deine WLAN-Zugangsdaten in den folgenden Zeilen ein:
        ```cpp
        const char* ssid = "DEIN_WLAN_NAME";
        const char* password = "DEIN_WLAN_PASSWORT";
        ```

3.  **Code hochladen / Upload the Code:**
    * Wähle das korrekte Board (z.B. "LOLIN(WEMOS) D1 R2 & mini") und den richtigen COM-Port aus.
    * Lade den Code auf den ESP8266 hoch.

4.  **IP-Adresse finden / Find the IP Address:**
    * Öffne den Seriellen Monitor in der Arduino IDE (Baudrate: 115200).
    * Nachdem sich der ESP mit dem WLAN verbunden hat, wird die IP-Adresse angezeigt.
        ```
        WLAN verbunden!
        IP-Adresse: 192.168.179.25
        ```

5.  **Weboberfläche öffnen / Open the Web Interface:**
    * Gib die angezeigte IP-Adresse in einen Webbrowser im selben Netzwerk ein. Du solltest nun die Weboberfläche der Waage sehen.

---

## Benutzung der Weboberfläche / Using the Web Interface

* **Kalibrierung:**
    1.  Stelle sicher, dass die Waage leer ist.
    2.  Öffne das "Kalibrierung"-Menü.
    3.  Klicke auf **"Tara setzen (Nullpunkt)"**. Der Rohwert sollte nun nahe Null sein.
    4.  Lege ein Gewicht mit bekanntem Wert auf die Waage (z.B. eine 1kg-Packung Mehl).
    5.  Gib das Gewicht **in Gramm** in das Feld "Kalibriergewicht" ein (z.B. `1000`).
    6.  Klicke auf **"Mit diesem Gewicht kalibrieren"**. Die Waage ist nun kalibriert und die Werte sind gespeichert.

* **MQTT-Einstellungen:**
    * Hier kannst du die IP-Adresse deines MQTT-Brokers, den Port und das Topic eintragen.
    * Aktiviere die Übertragung mit dem Haken bei "MQTT-Übertragung aktivieren".
    * Klicke auf **"MQTT-Einstellungen speichern"**, um die Daten zu sichern.

* **System:**
    * Der Button **"Werkseinstellungen wiederherstellen"** löscht alle gespeicherten Daten und startet die Waage mit den Standardwerten neu. Nützlich bei Konfigurationsproblemen.
