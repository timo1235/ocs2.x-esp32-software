# OCS2.x ESP32 Firmware - Umfassende Projektanalyse

> **Datum:** 16. Februar 2026
> **Analysierte Repositories:**
> - [ocs2.x-esp32-software](https://github.com/timo1235/ocs2.x-esp32-software) (Mainboard-Firmware, v1.1.5)
> - [ocs2.x-esp32-panel-software](https://github.com/timo1235/ocs2.x-esp32-panel-software) (Panel-Firmware, v2.0.3)

---

## Inhaltsverzeichnis

1. [Zusammenfassung](#1-zusammenfassung)
2. [Architektur-Übersicht](#2-architektur-übersicht)
3. [Kritische Probleme](#3-kritische-probleme)
4. [Hohe Priorität](#4-hohe-priorität)
5. [Mittlere Priorität](#5-mittlere-priorität)
6. [Niedrige Priorität](#6-niedrige-priorität)
7. [Bibliotheken-Analyse](#7-bibliotheken-analyse)
8. [Kommunikationsprotokoll Panel <-> Mainboard](#8-kommunikationsprotokoll-panel---mainboard)
9. [Sicherheitsanalyse](#9-sicherheitsanalyse)
10. [Community und Dokumentation](#10-community-und-dokumentation)
11. [Empfehlungen und Roadmap](#11-empfehlungen-und-roadmap)

---

## 1. Zusammenfassung

Das OCS2 ESP32 Firmware-Projekt ist ein funktionales, gut konzipiertes System für die Steuerung von Hobby-CNC-Maschinen. Die Architektur mit FreeRTOS-Task-Trennung, ESP-NOW für latenzarme Kommunikation und flexiblem Input-Mapping ist solide.

**Dennoch gibt es signifikante Probleme**, die in folgenden Bereichen liegen:

| Kategorie | Kritisch | Hoch | Mittel | Niedrig |
|-----------|----------|------|--------|---------|
| Mainboard-Firmware | 5 | 5 | 5 | 5 |
| Panel-Firmware | 1 | 2 | 3 | 2 |
| Protokoll/Integration | 2 | 2 | 1 | 1 |
| Sicherheit | 2 | 1 | 1 | - |
| Bibliotheken | 1 | 2 | 2 | 3 |

Die schwerwiegendsten Probleme betreffen **Race Conditions auf gemeinsam genutzten Datenstrukturen**, **fehlende Authentifizierung der Web-Oberfläche** und **fehlende Sicherheitsprüfungen beim Autosquaring**.

---

## 2. Architektur-Übersicht

### Systemtopologie

```
[ESP32 Panel/Handrad]  <-- WiFi (ESP-NOW) / Serial (RJ45 UART) -->  [ESP32 Mainboard (OCS2)]
[ESP32 Panel/Handrad]  <-- WiFi (ESP-NOW) -->                       [ColdEnd32 Kühlmittel]
```

### Mainboard FreeRTOS-Tasks (bis zu 10 Tasks auf 2 CPU-Kernen)

**CPU 0 (I/O-Kern):**
- `IO Task` - LED, Temperatur, Bounce-Inputs, Client-Daten
- `IO Task` - PCA9555/DAC-Ausgaben alle 20ms
- `IO Task` - PCA9555 auf Interrupt-Flags lesen
- `IO Task` - Periodische I2C-Chip-Verbindungsprüfung

**CPU 1 (Protokoll-Kern):**
- `Protocol task` - WiFi-Client-Timeout-Prüfung
- `Protocol serial task` - SerialTransfer Empfangen/Senden
- `Stepper task` - Autosquare-Button-Überwachung
- `GRBL JOGGING Task` - FluidNC-Jog-Befehle
- Diverse einmalige Setup-Tasks

### Panel FreeRTOS-Tasks

| Modul | Datei | Aufgabe |
|-------|-------|---------|
| `CONFIGMANAGER` | `configManager.cpp` | Persistente Konfiguration via NVS |
| `PROTOCOL` | `protocol.cpp` | ESP-NOW WiFi + UART Serial |
| `IOCONTROL` | `iocontrol.cpp` | GPIO-Lesen, Kalibrierung |
| `IOCONFIG` | `ioConfig.cpp` | GPIO-Abstraktion mit **per-Pin FreeRTOS-Tasks** |
| `DISPLAY` | `display.cpp` | SSD1306 OLED-Rendering bei 50Hz |
| `UIHANDLER` | `espui_handler.cpp` | WiFi AP + ESPUI Web-Konfiguration |

---

## 3. Kritische Probleme

### 3.1 Race Conditions auf globalen Datenstrukturen

**Dateien:** `src/protocol.cpp`, `src/iocontrol.cpp`
**Schweregrad:** KRITISCH

Die globalen Structs `dataToControl` und `dataToClient` werden von mehreren Tasks auf beiden CPU-Kernen **ohne jegliche Synchronisierung** gelesen und geschrieben.

```
ESP-NOW Callback (ISR-Kontext, beliebiger Kern) --> schreibt dataToControl
Serial Task (CPU 1)                             --> schreibt dataToControl
IO WriteOutputs Task (CPU 0)                    --> liest dataToControl
```

Es gibt keinen Mutex, kein Semaphore und keinen atomaren Zugriff. Dies kann zu **Torn Reads** führen, bei denen der IO-Task eine teilweise aktualisierte Struct liest. Konsequenz: korrupte Analogwerte zum DAC oder falsche Digital-Zustände.

**Besonders bedenklich:** In `protocol.h` (Zeile 119) existiert ein `static bool lockSending;`, das deklariert aber **nie definiert oder verwendet** wird - ein Hinweis darauf, dass Synchronisierung geplant aber nie implementiert wurde.

**Empfehlung:** Einen `SemaphoreHandle_t` für `dataToControl` und `dataToClient` einführen. Alle Schreib- und Lesezugriffe müssen den Mutex nehmen. Alternativ: Double-Buffering mit atomarem Pointer-Swap.

---

### 3.2 Client-Array Buffer Overflow

**Datei:** `src/protocol.cpp`, Zeilen 169-172
**Schweregrad:** KRITISCH

```cpp
bool PROTOCOL::addPeerIfNotExists(uint8_t *address) {
    PROTOCOL::clients[PROTOCOL::clientCount].integerAddress = ...;
    PROTOCOL::clientCount++;
}
```

Das `clients`-Array hat eine feste Größe von 5, aber es gibt **keine Bounds-Prüfung** vor der Indizierung mit `clientCount`. Wenn mehr als 5 ESP-NOW-Peers sich verbinden, wird über die Array-Grenzen hinaus geschrieben.

**Sicherheitsrelevanz:** Dies ist über WiFi ausnutzbar - ein Angreifer kann 6+ ESP-NOW-Pakete mit unterschiedlichen MAC-Adressen senden.

**Empfehlung:**
```cpp
if (PROTOCOL::clientCount >= 5) {
    DPRINTLN("Max clients reached, ignoring new peer");
    return false;
}
```

---

### 3.3 Keine Alarmprüfung während Autosquaring

**Datei:** `src/steppercontrol.cpp`, Funktion `autosquareProcess()`
**Schweregrad:** KRITISCH (Sicherheit)

Während des Autosquaring-Prozesses wird der `ALARMALL`-Eingang gelesen aber **nie geprüft**. Ein Alarm-Zustand (z.B. Not-Aus) stoppt die Motoren **nicht**. Die Motoren laufen weiter, bis der Benutzer den Autosquare-Button loslässt.

**Empfehlung:** In der Autosquare-Polling-Schleife eine Alarm-Prüfung einbauen:
```cpp
if (dataToClient.alarmState) {
    // Sofort alle Stepper stoppen
    for (auto& stepper : steppers) {
        if (stepper) stepper->forceStop();
    }
    break;
}
```

---

### 3.4 Kein maximales Verfahrlimit beim Autosquaring

**Datei:** `src/steppercontrol.cpp`
**Schweregrad:** KRITISCH (Sicherheit)

Wenn ein Endschalter nicht angeschlossen, defekt oder falsch konfiguriert ist, laufen die Motoren **unbegrenzt**. Es gibt kein Software-Verfahrlimit als Sicherheitsnetz.

**Empfehlung:** Einen konfigurierbaren maximalen Verfahrweg einführen (z.B. `maxTravelDistance_mm`). Nach Überschreiten dieses Limits Motoren stoppen und Fehler anzeigen.

---

### 3.5 Keine Web-Authentifizierung

**Datei:** `src/configManager.cpp`
**Schweregrad:** KRITISCH (Sicherheit)

Die Web-Konfiguration unter `/cfg` hat **kein Login, kein Passwort, kein Session-Management**. Jeder im WiFi-Netzwerk kann alle Parameter der CNC-Steuerung ändern. Zusätzlich:

- AP-Modus erstellt ein **offenes WiFi-Netzwerk** (kein WPA-Passwort)
- OTA-Firmware-Upload (über ConfigAssist) hat keine Authentifizierung
- Der `/d`-Endpoint gibt alle Konfigurationswerte aus, **inklusive WiFi-Passwort**
- Das WiFi-Passwort wird auch auf Serial ausgegeben (`printMainConfig()`)

**Empfehlung:** Mindestens HTTP Basic Auth für alle Web-Endpunkte einführen. AP-Modus sollte ein Standard-Passwort verwenden (z.B. `OCS2-{ChipID}`).

---

### 3.6 Kein gemeinsames Protokoll-Library zwischen Panel und Mainboard

**Dateien:** Mainboard `src/protocol.h` vs. Panel `include/protocol.h`
**Schweregrad:** KRITISCH

Die Struct-Definitionen (`DATA_TO_CONTROL`, `DATA_COMMAND`, `DATA_TO_CLIENT`) werden in beiden Repositories **unabhängig gepflegt**. Es existiert bereits eine Namensinkonsistenz:

| Feld | Mainboard | Panel |
|------|-----------|-------|
| Programm-Start | `programmStart` (doppel-m) | `programStart` (einfach-m) |
| SET-Flag | `setProgrammStart` | `setProgramStart` |

Da die Structs als **Raw-Bytes über ESP-NOW übertragen** werden, führt jede strukturelle Änderung in einem Repo (Feld hinzufügen, Reihenfolge ändern) zu **stillem Daten-Korruption** im anderen.

**Empfehlung:** Ein gemeinsames Git-Submodule oder eine separate Shared-Library für Protokoll-Definitionen erstellen.

---

## 4. Hohe Priorität

### 4.1 MAC-Adress-"Hash" mit massivem Kollisionsrisiko

**Datei:** `src/protocol.cpp`, Zeilen 537-546

```cpp
uint16_t PROTOCOL::getIntegerFromAddress(const uint8_t *address) {
    uint16_t integer = 0;
    integer += address[0];
    integer += address[1];
    // ... alle 6 Bytes summiert
    return integer;
}
```

Die Funktion **summiert** die sechs MAC-Adress-Bytes zu einem `uint16_t`. Das ist kein Hash - es ist eine einfache Summe mit massivem Kollisionspotential. `01:02:03:04:05:06` und `06:05:04:03:02:01` erzeugen denselben Wert. In der Praxis ist das Risiko durch das feste MAC-Format (`5E:00:00:00:XX:02`) begrenzt, aber die Funktion ist fragil.

**Empfehlung:** Direkte 6-Byte MAC-Vergleiche mit `memcmp()` verwenden statt eines Summen-"Hash".

---

### 4.2 Fehlende `volatile`-Deklaration auf ISR-geteilten Variablen

**Datei:** `src/iocontrol.cpp`, Zeilen 14-17

```cpp
bool ioPort1Flag = false;     // gesetzt in ISR, gelesen in Task
bool ioPort2Flag = false;     // gesetzt in ISR, gelesen in Task
bool functionButtonPressedFlag = false;  // gesetzt in ISR, gelesen in Task
```

Ohne `volatile` kann der Compiler Lesezugriffe in Register-Cache optimieren. Der Task sieht dann möglicherweise nie das ISR-Update. Auf dem Dual-Core ESP32 ist dies besonders gefährlich.

**Empfehlung:** `volatile bool` verwenden oder `std::atomic<bool>`.

---

### 4.3 Dangling Pointer für WiFi-Hostname

**Datei:** `src/configManager.cpp`, Zeilen 106-108

```cpp
mainConfig.wifiConfig.hostname = conf["WiFi_Hostname"].c_str();
```

`c_str()` gibt einen Zeiger auf den internen Buffer des temporären `String`-Objekts zurück. Wenn ConfigAssist seinen internen Speicher umallokiert, wird dieser Zeiger **ungültig**. Kann zu Abstürzen bei `WiFi.setHostname()` führen.

**Empfehlung:** Den Hostname als `String` (nicht `const char*`) in der `WIFI_CONFIG`-Struct speichern, analog zu `ssid` und `pass`.

---

### 4.4 Unchecked `std::map::find()` Ergebnisse

**Datei:** `src/configManager.cpp`, Zeile 49

```cpp
mainConfig.versionInfo.boardType = boardTypeMap.find(conf["PCB_Type"].c_str())->second;
```

Wenn `conf["PCB_Type"]` einen Wert liefert, der nicht in `boardTypeMap` existiert, gibt `find()` den `end()`-Iterator zurück. Das Dereferenzieren von `end()->second` ist **Undefined Behavior** (wahrscheinlich ein Absturz). Dieses Muster wiederholt sich für alle `axisMap`-Lookups.

**Empfehlung:**
```cpp
auto it = boardTypeMap.find(conf["PCB_Type"].c_str());
if (it != boardTypeMap.end()) {
    mainConfig.versionInfo.boardType = it->second;
} else {
    DPRINTLN("Unknown board type, using default");
    mainConfig.versionInfo.boardType = BOARD_TYPE::undefined;
}
```

---

### 4.5 Division durch Null bei Stepper-Initialisierung

**Datei:** `src/steppercontrol.cpp`, Zeilen 109-111

```cpp
int stepsPerMM = autosquareConfigs[configIndex].stepsPerRevolution
                / autosquareConfigs[configIndex].mmPerRevolution;
```

Wenn `mmPerRevolution` auf 0 konfiguriert wird (das Web-UI erlaubt dies - kein `min`-Attribut), erfolgt eine **Division durch Null** mit Absturz.

**Empfehlung:** Validierung beim Config-Laden und vor der Berechnung.

---

### 4.6 Kein Protokoll-Versionierungs-Handshake

**Dateien:** Beide Repositories

Das `softwareVersion`-Feld existiert in `DATA_TO_CONTROL` und `DATA_TO_CLIENT`, aber es gibt **keine Kompatibilitätsprüfung**. Das Panel sendet Version 3, das Mainboard initialisiert Version 2. Es gibt keinen Handshake, um Inkompatibilitäten zu erkennen. Da Structs als `memcpy`-Raw-Bytes übertragen werden, führt ein Struct-Layout-Unterschied zu stiller Korruption.

**Empfehlung:** Beim ersten Datenempfang die Version prüfen und bei Inkompatibilität eine Warnung anzeigen (z.B. über LED-Blinkmuster oder Display-Meldung).

---

## 5. Mittlere Priorität

### 5.1 WiFi-Verbindung mit Endlosschleife

**Datei:** `src/configManager.cpp`, Zeilen 240-243

```cpp
void CONFIGMANAGER::setupWiFiConnect() {
    WiFi.begin(mainConfig.wifiConfig.ssid, mainConfig.wifiConfig.pass);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
}
```

Wenn die konfigurierte WiFi-SSID nicht erreichbar ist, **blockiert dieser Task für immer**. Kein Timeout, kein Retry-Limit, kein Fallback auf AP-Modus.

**Empfehlung:** Timeout von 30 Sekunden einbauen, danach Fallback auf AP-Modus.

---

### 5.2 Operator-Priorität in Board-Typ-Prüfungen

**Datei:** `src/iocontrol.cpp`, z.B. Zeile 41

```cpp
if (versionManager.isBoardType(BOARD_TYPE::OCS2) && versionManager.isHigherThan(2, 9)
    || versionManager.isBoardType(BOARD_TYPE::OCS2_Mini)) {
```

Durch C++ Operator-Priorität (`&&` bindet stärker als `||`) wird dies als `(OCS2 && >2.9) || OCS2_Mini` evaluiert. Das ist vermutlich beabsichtigt, aber ohne Klammern mehrdeutig. Dieses Muster taucht ca. 6 mal im Code auf.

**Empfehlung:** Explizite Klammern setzen: `(isBoardType(OCS2) && isHigherThan(2,9)) || isBoardType(OCS2_Mini)`.

---

### 5.3 Keine Server-seitige Konfigurationsvalidierung

**Datei:** `src/configManager.cpp`

Alle Konfigurationswerte aus dem Web-Formular werden blind vertraut:

- `stepper_acceleration`, `stepsPerRevolution`, `mmPerRevolution` akzeptieren 0 und negative Werte -> **Abstürze**
- Motor-Achsen-Zuweisungen werden nicht gegen den Board-Typ validiert (Achse C auf OCS2_Mini möglich, aber Stepper ist NULL)
- Endschalter-Eingänge können doppelt zugewiesen werden
- HTML `min`/`max`-Attribute sind nur Client-seitig - ein direkter HTTP POST umgeht sie

**Empfehlung:** Server-seitige Validierung aller numerischen Werte nach dem Config-Laden.

---

### 5.4 Tight Spin Loop beim Autosquare-Endschalter-Polling

**Datei:** `src/steppercontrol.cpp`

Die Endschalter-Polling-Schleife im Autosquaring hat kein `vTaskDelay()` und läuft als enger Spin-Loop. Dies kann:
- Watchdog-Timer-Resets auf dem ESP32 auslösen
- Andere Tasks auf demselben CPU-Kern aushungern
- Übermäßige I2C-Bus-Last erzeugen

**Empfehlung:** `vTaskDelay(1)` oder `taskYIELD()` in die Polling-Schleife einbauen.

---

### 5.5 Per-GPIO FreeRTOS-Tasks im Panel

**Datei:** Panel `ioConfig.cpp`

Jeder analoge Pin und jeder entprellte Button bekommt seinen **eigenen FreeRTOS-Task** via `GPIO_CLASS::updateTask`. Bei 5 analogen Eingängen und ~12 digitalen Buttons ergibt das bis zu 17 Tasks mit je 1500 Byte Stack. Das ist verschwenderisch bezüglich RAM und Scheduler-Overhead.

**Empfehlung:** Einen einzelnen Polling-Task verwenden, der alle GPIO-Pins in einer Schleife abfragt.

---

## 6. Niedrige Priorität

### 6.1 Hostname-Tippfehler

**Datei:** `src/configManager.cpp`, Zeile 30

```cpp
{ "", "", "OSC2"}  // Sollte "OCS2" sein (Buchstaben vertauscht)
```

### 6.2 Alle IO-Tasks haben denselben Namen

**Datei:** `src/iocontrol.cpp`

Vier verschiedene Tasks heißen alle `"IO Task"`, was Debugging mit FreeRTOS-Task-Monitoren extrem erschwert.

### 6.3 Leere Callback- und Loop-Funktionen

- `PROTOCOL::onDataSent()` hat einen leeren Body - Send-Erfolg/Fehler-Zähler werden nie aktualisiert
- `IOCONTROL::loop()` ist leer und wird nie aufgerufen
- `lastOutputUpdate` (Zeile 35, configManager.cpp) wird nie gelesen

### 6.4 Debug-Ausgabe immer aktiv

Beide Projekte definieren `OCS_DEBUG` standardmäßig als aktiviert. Debug-Serial-Output ist in Release-Builds immer aktiv, was CPU-Zeit verbraucht.

### 6.5 `getMacStrFromAddress` gibt statischen Buffer zurück

**Datei:** `src/protocol.cpp`, Zeilen 530-535

Gibt einen Zeiger auf einen `static char macStr[18]` zurück. Wenn die Funktion zweimal im selben Ausdruck aufgerufen wird, überschreibt der zweite Aufruf das erste Ergebnis. Aktuell sicher, aber wartungsgefährdend.

### 6.6 Magic Numbers

Verstreut im gesamten Code:
- Stack-Größen hardcoded (4096, 2048) ohne Konstanten
- Timeout-Werte wie `5000`, `20000` ohne Erklärung
- `0x5E` als MAC-Adress-Byte
- `125` als LED-PWM-Wert
- `DAC_JOYSTICK_RESET_VALUE` ist `1023 / 2 = 511` (Integer-Division)

---

## 7. Bibliotheken-Analyse

### 7.1 BU2506FV DAC Library v0.0.1

**Pfad:** `lib/BU2506FV/`
**Status:** Experimentell (eigene Aussage im README)

**Probleme:**

| Problem | Schwere | Beschreibung |
|---------|---------|--------------|
| Buffer Overflow | KRITISCH | `_value[channel] = data;` für channel=8 schreibt auf Index 8 bei Array-Größe 8 (gültige Indices: 0-7) |
| Inkonsistente Bit-Shifts | Hoch | Channel 1 (default case) wendet `>> 2` Shift nicht an, alle anderen Channels aber schon |
| Kein NULL-Init | Mittel | `mySPI`-Pointer wird im Konstruktor nicht initialisiert |
| Falsche Beispiele | Niedrig | Examples-Verzeichnis enthält MCP4921-Beispiele (anderer DAC-Chip) |
| Repository 404 | Niedrig | Verlinktes Upstream-Repo `timo1235/bu2506fv-dac` existiert nicht mehr |

### 7.2 LEDController (Custom)

**Pfad:** `lib/LEDController/`

| Problem | Schwere | Beschreibung |
|---------|---------|--------------|
| Hardcoded LEDC Channel | Mittel | Immer Channel 0 - Konflikte wenn andere LEDs PWM benötigen |
| `busy` nie initialisiert | Mittel | `getBusy()` gibt Garbage zurück |
| `loop()` ist leer | Niedrig | `busy`, `startTime`, `blinkTime` deklariert aber nie verwendet |
| Deprecated API | Niedrig | Nutzt `ledcSetup`/`ledcAttachPin` statt neuerem `ledcAttach()` |

### 7.3 PCA9555 I2C I/O Expander v1.0

**Pfad:** `lib/PCA9555/`
**Original:** Nico Verduin (2015), für AVR geschrieben

| Problem | Schwere | Beschreibung |
|---------|---------|--------------|
| `digitalReadAll()` fehlt | Hoch | In Header deklariert, nie implementiert - Linker-Fehler wenn aufgerufen |
| `DEBUG 1` aktiv | Mittel | Debug-Output immer eingeschaltet |
| Nur 1 Interrupt-Instanz | Mittel | Statischer `instancePointer` erlaubt nur einem PCA9555 Interrupts |
| Keine I2C-Retry-Logik | Mittel | Einzelne fehlerhafte Reads breiten sich unkontrolliert aus (relevant für Issue #8) |
| `architectures=avr` | Niedrig | library.properties gibt AVR an, wird aber auf ESP32 genutzt |

### 7.4 ConfigAssist v2.8.7 (Lokale Kopie)

**Pfad:** `lib/ConfigAssist2.8.7/`

Die Upstream-Version ist identisch (v2.8.7), aber die `platformio.ini` erwähnt "some changes in the lib". Die Änderungen sind **nicht markiert** (kein MODIFIED/CUSTOM/HACK-Kommentar), was ein zukünftiges Upstream-Merge erschwert.

### 7.5 Abhängigkeiten-Status

| Bibliothek | Pinned | Aktuell | Status |
|------------|--------|---------|--------|
| FastAccelStepper | ^0.31.5 | 0.33.5 | **2 Minor-Versionen zurück** |
| Bounce2 | ^2.72.0 | ~2.72.0 | Aktuell |
| DallasTemperature | ^4.0.4 | ~4.0.4 | Aktuell |
| SerialTransfer | ^3.1.3 | ~3.1.3 | Aktuell |
| Improv WiFi Library | **nicht gepinnt** | Latest | **Risiko: Breaking Changes** |
| ConfigAssist | 2.8.7 (lokal) | 2.8.7 | Aktuell |

**Risiko:** Die Improv WiFi Library hat **keine Version-Pinning** - Breaking Changes können still eingezogen werden.

---

## 8. Kommunikationsprotokoll Panel <-> Mainboard

### Datenfluss

```
Panel --> DATA_TO_CONTROL (~32 Bytes) --> Mainboard
         Joystick XYZ, Feedrate, Rotation Speed,
         Button States, Command Flags

Mainboard --> DATA_TO_CLIENT (~34 Bytes) --> Panel
              Temperaturen[5], Autosquare-Status,
              Spindel-Status, Alarm-Status
```

### Transport

| Eigenschaft | ESP-NOW (WiFi) | Serial (UART/RJ45) |
|-------------|----------------|---------------------|
| Baud/Rate | ~1 Mbit/s | 115200 Baud |
| Latenz | ~2-5ms | ~1ms |
| Reichweite | ~50m (Sichtlinie) | Kabellänge |
| Verschlüsselung | **Keine** | N/A (physisch) |
| Max. Clients | 5 | 1 |
| Priorität | Niedrig | **Hoch** (überschreibt WiFi) |
| Intervall | 50ms (konfigurierbar) | 50ms (konfigurierbar) |
| Timeout | 4 * Intervall | 4 * Intervall |

### Identifizierte Protokoll-Probleme

1. **Keine Empfangsbestätigung für ESP-NOW-Sends an Panels** - `onDataSent` Callback ist leer
2. **Serial-Polling bei 10ms** - Bei 115200 Baud könnte der Buffer überlaufen wenn das Panel schneller sendet
3. **Kein Shared Library** - Struct-Definitionen unabhängig in beiden Repos gepflegt
4. **Naming-Inkonsistenz** - `programmStart` vs. `programStart` (binär kompatibel, aber wartungsgefährdend)
5. **Temperatur-Array-Diskrepanz** - Mainboard füllt 5 Werte, Panel-Display zeigt nur 2
6. **ColdEnd-Protokoll nur im Panel** - Mainboard hat keine Sichtbarkeit auf Kühlsystem-Status

---

## 9. Sicherheitsanalyse

### Netzwerksicherheit

| Bedrohung | Status | Risiko |
|-----------|--------|--------|
| Web-Interface ohne Auth | **Ungeschützt** | KRITISCH |
| Offenes WiFi-AP | **Ungeschützt** | KRITISCH |
| OTA ohne Auth | **Ungeschützt** | HOCH |
| ESP-NOW unverschlüsselt | **Ungeschützt** | MITTEL |
| WiFi-Passwort auf Serial | **Exponiert** | MITTEL |
| `/d` Endpoint leakt Credentials | **Exponiert** | MITTEL |
| Vorhersagbare MAC-Adressen | **Schwach** | NIEDRIG |

### CNC-Maschinensicherheit

| Bedrohung | Status | Risiko |
|-----------|--------|--------|
| Kein Alarm-Check bei Autosquare | **Kein Schutz** | KRITISCH |
| Kein Verfahrlimit bei Autosquare | **Kein Schutz** | KRITISCH |
| Endschalter ohne Entprellung im Autosquare | **Unzuverlässig** | MITTEL |
| Race Conditions auf Steuerungsdaten | **Ungeschützt** | KRITISCH |

### Kontext-Einordnung

Dies ist ein Hobby-CNC-Projekt. Die Bedrohungslandschaft ist anders als bei industriellen CNC-Maschinen. Dennoch: Ein unbeabsichtigtes Verfahren ohne Stopp kann zu **Hardwareschäden** (gebrochene Spindel, verformte Linearführungen) und in Extremfällen zu **Verletzungen** führen (rotierende Fräser, schnell verfahrende Achsen). Daher sollten die maschinensicherheitsbezogenen Probleme prioritär behandelt werden.

---

## 10. Community und Dokumentation

### Repository-Metriken

| Metrik | Mainboard | Panel |
|--------|-----------|-------|
| Stars | 5 | 9 |
| Forks | 6 | 3 |
| Commits | 51 | 28 |
| Offene Issues | 3 | 0 |
| Letztes Release | Mai 2025 | Aug 2023 |
| Externe PRs | 2 von 7 | 1 von 2 |

### Muster aus geschlossenen Issues

Die Mehrheit der Issues betrifft **Konfigurationsverwirrung**, nicht Software-Bugs:
- Endschalter-Polarität und -zuordnung für Autosquaring
- Welche Inputs für Autosquaring nutzbar sind (nur 1-8)
- Warum das Panel nur mit laufendem Estlcam funktioniert
- I2C-Hardware-Fehler (defekte PCA9555-Chips)

### Dokumentation

Die Dokumentationsseite `docs.timos-werkstatt.de` ist **noch im Aufbau** ("Auf dieser Seite entsteht eine Dokumentation"). Es fehlen:
- Detaillierte ESP32-Firmware-Konfigurationsanleitungen
- Troubleshooting-Guides für häufige Fehler
- Autosquaring-Setup Schritt-für-Schritt
- Input-Zuweisungs-Mapping (welche Inputs wofür nutzbar)

### Unbeantwortete Issues

- **Issue #19** (Rückwärtslauf Rapidchange) - seit 6 Wochen ohne Antwort
- **Issue #11** (Estlcam über WiFi) - seit 20 Monaten offen, als "enhancement" getaggt

---

## 11. Empfehlungen und Roadmap

### Phase 1: Kritische Fixes (Sofort)

1. **Mutex für `dataToControl`/`dataToClient` einführen** - Behebt Race Conditions
2. **Bounds-Check in `addPeerIfNotExists()`** - Verhindert Buffer Overflow
3. **Alarm-Check in `autosquareProcess()` einbauen** - Maschinensicherheit
4. **Maximales Verfahrlimit für Autosquaring** - Maschinensicherheit
5. **BU2506FV Buffer Overflow fixen** - Array-Index-Prüfung für Channel 8

### Phase 2: Hohe Priorität (Kurzfristig)

6. **HTTP Basic Auth für Web-Interface** - Minimale Sicherheit
7. **Server-seitige Config-Validierung** - Keine Null/Negativ-Werte für Stepper-Params
8. **`volatile` auf ISR-Flags** - Korrektes Multi-Core-Verhalten
9. **WiFi-Verbindungs-Timeout** - Fallback statt Endlosschleife
10. **Improv WiFi Library Version pinnen** - Reproduzierbare Builds
11. **Shared Protocol Library** als Git-Submodule erstellen

### Phase 3: Mittlere Priorität (Mittelfristig)

12. **FastAccelStepper aktualisieren** - Bug-Fixes und Verbesserungen
13. **ConfigAssist-Modifikationen dokumentieren** - Upstream-Merge vorbereiten
14. **I2C Retry-Logik in PCA9555** - Robustheit bei transienten Fehlern
15. **vTaskDelay in Autosquare-Polling** - Watchdog und Task-Scheduling
16. **Panel: GPIO-Tasks konsolidieren** - RAM und Scheduler-Overhead reduzieren
17. **CI/CD Pipeline** (GitHub Actions) für automatisierte Builds

### Phase 4: Niedrige Priorität (Langfristig)

18. **Firmware-Version auf Flash-Seite anzeigen** statt nur "latest"
19. **Hostname-Tippfehler korrigieren** (`OSC2` -> `OCS2`)
20. **IO-Task-Namen eindeutig machen** für besseres Debugging
21. **Dead Code entfernen** (`lockSending`, leere Callbacks, ungenutzte Variablen)
22. **Debug-Output standardmäßig deaktivieren** in Release-Builds
23. **LEDController überarbeiten** - Initialisierung, LEDC-Channel-Konflikte
24. **Deprecated LEDC API** auf neues `ledcAttach()` migrieren
25. **Dokumentation ausbauen** - Insbesondere Autosquaring und Input-Mapping

---

## Fazit

Das OCS2 ESP32 Firmware-Projekt ist eine beeindruckende Solo-Entwickler-Leistung für ein Nischen-Produkt. Die Architektur-Entscheidungen (FreeRTOS, ESP-NOW, modulares Design) sind durchdacht und die Hardware-Abstraktion ist für die Komplexität der unterstützten Board-Varianten bemerkenswert.

Die identifizierten Probleme lassen sich in drei Kategorien zusammenfassen:

1. **Maschinensicherheit** - Fehlende Schutzmaßnahmen beim Autosquaring (Alarm, Verfahrlimit) sollten höchste Priorität haben
2. **Datenintegrität** - Race Conditions und fehlende Synchronisierung können zu unvorhersagbarem Verhalten führen
3. **Wartbarkeit** - Getrennte Protokoll-Definitionen, undokumentierte Library-Patches und fehlende CI/CD erschweren die langfristige Wartung

Die meisten dieser Probleme sind mit überschaubarem Aufwand behebbar und würden die Qualität und Zuverlässigkeit des Projekts signifikant steigern.
