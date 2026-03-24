# s7prtgsensor

PRTG-Sensor zum Auslesen von Siemens SIMATIC S7-SPSen (S7-300, S7-400, S7-1200, S7-1500) über die snap7-Bibliothek. Läuft als nativer **EXE/Script Advanced**-Sensor unter Windows und gibt PRTG-kompatibles XML auf stdout aus.

---
## Download

Das aktuelle Release v0.9.0 kann als Binary hier heruntergeladen werden: https://github.com/mirkowi/s7prtgsensor/releases/download/v0.9.0/s7sensor-windows-x64.zip

## Voraussetzungen

| Komponente | Quelle |
|---|---|
| snap7.dll / snap7.lib / snap7.h | [github.com/davenardella/snap7](https://github.com/davenardella/snap7) – wird beim CI-Build automatisch geladen |
| nlohmann/json.hpp | [github.com/nlohmann/json](https://github.com/nlohmann/json) – wird beim CI-Build automatisch geladen |
| CMake ≥ 3.20 | cmake.org |
| Compiler | Visual Studio 2022 oder MSYS2/MinGW-w64 |

**SPS-seitig:** PUT/GET-Kommunikation muss in TIA Portal aktiviert sein:
*CPU-Eigenschaften → Schutz & Sicherheit → Verbindungsmechanismen → „Zugriff über PUT/GET-Kommunikation erlauben"*

---

## Build

### Visual Studio (MSVC)
```bat
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### MinGW / MSYS2
```bat
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Das Ergebnis sind zwei Dateien: `s7sensor.exe` und `snap7.dll`.

---

## PRTG-Deployment

1. `s7sensor.exe` und `snap7.dll` → `%PRTG_PROGRAM_ROOT%\Custom Sensors\EXEXML\`
2. Konfigurationsdatei (z. B. `plc1.json`) ebenfalls dorthin kopieren
3. Neuen Sensor anlegen: Typ **EXE/Script Advanced**
4. EXE: `s7sensor.exe`
5. Parameter: `--config "%scriptlocation%\plc1.json"`
6. Sensor-Timeout: ≥ 10 Sekunden

---

## Programmaufruf

```
s7sensor.exe [Optionen]
```

| Option | Beschreibung |
|---|---|
| `--config <pfad>` | Pfad zur JSON-Konfigurationsdatei. Standard: `s7sensor.json` im selben Verzeichnis wie die EXE. |
| `--ip <adresse>` | Überschreibt die IP-Adresse aus der Konfigurationsdatei. |
| `--rack <n>` | Überschreibt die Rack-Nummer aus der Konfigurationsdatei. |
| `--slot <n>` | Überschreibt die Slot-Nummer aus der Konfigurationsdatei. |
| `--debug` | Aktiviert den Debug-Log (`s7sensor_debug.log` neben der EXE). Niemals auf stdout. |
| `--list-szl` | Verbindet mit der SPS, gibt alle verfügbaren SZL-IDs aus und beendet das Programm. |
| `--dump-szl` | Liest **jede** verfügbare SZL aus und gibt den Inhalt als Hex-Dump aus. Zeigt pro SZL: ID, LENTHDR, N_DR und Rohdaten. Nützlich zur Ermittlung von Byte-Offsets für die Konfiguration. |
| `--read-szl <id>` | Liest eine **bestimmte** SZL aus und gibt den Hex-Dump aus. `<id>` kann dezimal oder hexadezimal (z. B. `0x0074`) angegeben werden. |
| `--szl-index <n>` | Index für `--read-szl` (Standard: `0`). |
| `--szl-offset <n>` | Startet den Hex-Dump ab Byte-Offset `<n>` (Standard: `0`). Nützlich um gezielt Datensätze ab einem bestimmten Byte zu analysieren. |
| `--help` | Zeigt Kurzhilfe an. |

### Beispiele

```bat
rem Normaler Sensoraufruf
s7sensor.exe --config "C:\PRTG\plc1.json"

rem IP aus der Config überschreiben
s7sensor.exe --config plc1.json --ip 10.0.0.42

rem Verfügbare SZL-IDs der SPS anzeigen
s7sensor.exe --config plc1.json --list-szl

rem Alle SZLs auslesen und als Hex-Dump ausgeben (Byte-Offsets ermitteln)
s7sensor.exe --config plc1.json --dump-szl

rem Einzelne SZL 0x0074 (Diagnosepuffer) ab Byte 0 ausgeben
s7sensor.exe --config plc1.json --read-szl 0x0074

rem SZL 0x0074 ab Byte 4 ausgeben (überspringt LENTHDR+N_DR-Header)
s7sensor.exe --config plc1.json --read-szl 0x0074 --szl-offset 4

rem SZL mit bestimmtem Index und Offset
s7sensor.exe --config plc1.json --read-szl 0x0131 --szl-index 1 --szl-offset 0

rem Detailliertes Debugging
s7sensor.exe --config plc1.json --debug
```

Der Exit-Code ist immer **0** – PRTG wertet ausschließlich das XML aus.

---

## Konfigurationsdatei (JSON)

### Struktur

```json
{
  "connection": { ... },
  "channels":   [ ... ]
}
```

---

### `connection`

| Feld | Typ | Standard | Beschreibung |
|---|---|---|---|
| `ip` | string | – | IP-Adresse der SPS (**Pflichtfeld**) |
| `rack` | int | `0` | Rack-Nummer |
| `slot` | int | `1` | Slot-Nummer (S7-300: `2`, S7-1200/1500: `1`) |
| `timeout_ms` | int | `3000` | Verbindungs-Timeout in Millisekunden |

```json
"connection": {
  "ip": "192.168.1.10",
  "rack": 0,
  "slot": 1,
  "timeout_ms": 3000
}
```

---

### `channels` – gemeinsame Felder

Jedes Element in `channels` beschreibt einen PRTG-Kanal.

| Feld | Typ | Standard | Beschreibung |
|---|---|---|---|
| `name` | string | – | Kanalname in PRTG (**Pflichtfeld**) |
| `address` | string | – | Adresse des Datenwerts (**Pflichtfeld**, siehe unten) |
| `datatype` | string | `"BYTE"` | Datentyp des Werts (siehe unten). Bei String-Kanälen (CPU.Info.*) nicht erforderlich. |
| `scale_factor` | float | `1.0` | Multiplikator: `Anzeigewert = Rohwert × scale_factor + scale_offset` |
| `scale_offset` | float | `0.0` | Addierter Offset nach der Skalierung |
| `float_decimals` | int | `0` | Nachkommastellen in PRTG. `0` = Ganzzahl-Ausgabe. |
| `unit` | string | `"Custom"` | PRTG-Einheit (siehe Tabelle unten) |
| `customunit` | string | `""` | Eigene Einheitsbezeichnung, wenn `unit` = `"Custom"` |
| `limits_enabled` | bool | `false` | Grenzwert-Überwachung aktivieren |
| `limit_min_error` | float | – | Unterer Fehler-Grenzwert |
| `limit_min_warning` | float | – | Unterer Warn-Grenzwert |
| `limit_max_warning` | float | – | Oberer Warn-Grenzwert |
| `limit_max_error` | float | – | Oberer Fehler-Grenzwert |

#### Unterstützte PRTG-Einheiten (`unit`)

| Wert | Bedeutung |
|---|---|
| `"Temperature"` | Temperatur (°C/°F je nach PRTG-Einstellung) |
| `"Percent"` | Prozent |
| `"BytesBandwidth"` | Bytes/s |
| `"BytesMemory"` | Bytes (Speicher) |
| `"BytesDisk"` | Bytes (Festplatte) |
| `"Count"` | Anzahl |
| `"CPU"` | CPU-Auslastung (%) |
| `"TimeResponse"` | Antwortzeit (ms) |
| `"TimeHours"` | Stunden |
| `"Custom"` | Eigene Einheit (mit `customunit` kombinieren) |

---

### Unterstützte Datentypen (`datatype`)

| Wert | Größe | Wertebereich |
|---|---|---|
| `"BOOL"` | 1 Bit | 0 oder 1 |
| `"BYTE"` | 1 Byte | 0 … 255 |
| `"WORD"` | 2 Byte | 0 … 65535 |
| `"DWORD"` | 4 Byte | 0 … 4294967295 |
| `"INT"` | 2 Byte | −32768 … 32767 |
| `"DINT"` | 4 Byte | −2147483648 … 2147483647 |
| `"REAL"` | 4 Byte | IEEE 754 Gleitkomma |
| `"CHAR"` | 1 Byte | −128 … 127 |

---

### Unterstützte Adressen (`address`)

#### Datenbaustein (DB)

| Adresse | Bedeutung |
|---|---|
| `DB5.DBX0.3` | DB5, Byte 0, Bit 3 → `datatype: "BOOL"` |
| `DB5.DBB4` | DB5, Byte 4 |
| `DB5.DBW6` | DB5, Wort ab Byte 6 (2 Byte) |
| `DB5.DBD10` | DB5, Doppelwort ab Byte 10 (4 Byte, auch für REAL) |

#### Merker (M)

| Adresse | Bedeutung |
|---|---|
| `M0.1` | Merkerbit Byte 0, Bit 1 → `datatype: "BOOL"` |
| `MB50` | Merkerbyte 50 |
| `MW100` | Merkerwort ab Byte 100 |
| `MD200` | Merkerdoppelwort ab Byte 200 |

#### Eingänge / Ausgänge

| Adresse | Bedeutung |
|---|---|
| `E0.0` / `I0.0` | Digitaleingang Byte 0, Bit 0 |
| `EB0` / `IB0` | Eingangsbyte 0 |
| `EW0` / `IW0` | Eingangswort ab Byte 0 |
| `A0.0` / `Q0.0` | Digitalausgang Byte 0, Bit 0 |
| `AB0` / `QB0` | Ausgangsbyte 0 |
| `AW0` / `QW0` | Ausgangswort ab Byte 0 |

#### CPU-Sonderkanäle

| Adresse | Datentyp | Beschreibung |
|---|---|---|
| `CPU.Status` | `WORD` | Betriebszustand: `1`=RUN, `0`=STOP, `2`=UNKNOWN. Erkennt **keine** Fehler im RUN-Modus (BF-LED, OB85 usw.). |
| `CPU.DiagBufCount` | `WORD` | Anzahl Einträge im Diagnosepuffer. Steigt bei jedem Diagnoseereignis. Für Fehlererkennung im RUN-Modus zusammen mit `CPU.Status` verwenden. |
| `CPU.Info.ModuleTypeName` | – | CPU-Typ-Bezeichnung (string, kein `datatype` nötig) |
| `CPU.Info.SerialNumber` | – | Seriennummer (string) |
| `CPU.Info.ASName` | – | AS-Name / Programmname (string) |
| `CPU.Info.ModuleName` | – | Modulname (string) |
| `CPU.Info.Copyright` | – | Copyright-String (string) |

> String-Kanäle (`CPU.Info.*`) erscheinen in PRTG mit dem Wert `1` und dem Text als Einheit, da PRTG keine echten String-Kanäle unterstützt.

#### SZL (Systemzustandsliste)

```
SZL.<ID>.<Index>.<ByteOffset>
```

Liest einen Rohwert aus der Systemzustandsliste der SPS.

| Parameter | Beschreibung |
|---|---|
| `ID` | SZL-ID als Dezimal- oder Hex-Zahl (z. B. `0x0074`) |
| `Index` | SZL-Index (meist `0` für alle Einträge) |
| `ByteOffset` | Byte-Position innerhalb der Antwort |

**Byte-Layout der Antwort** (einheitlich für alle SZL-Reads):

| Byte | Typ | Inhalt |
|---|---|---|
| 0–1 | WORD | `LENTHDR` – Länge eines Datensatzes |
| 2–3 | WORD | `N_DR` – Anzahl Datensätze |
| 4+ | – | Eigentliche SZL-Datensätze |

Verfügbare SZL-IDs abfragen:
```bat
s7sensor.exe --config plc1.json --list-szl
```

> Nicht alle SZL-IDs sind auf jeder CPU verfügbar. Fehler `0x00C00000` (`errCliItemNotAvailable`) bedeutet, dass die angeforderte ID nicht existiert.

---

### Vollständiges Konfigurationsbeispiel

```json
{
  "connection": {
    "ip": "192.168.1.10",
    "rack": 0,
    "slot": 1,
    "timeout_ms": 3000
  },
  "channels": [
    {
      "name": "CPU Status",
      "address": "CPU.Status",
      "datatype": "WORD",
      "unit": "Custom",
      "customunit": "RUN=1/STOP=0/UNKNOWN=2",
      "limits_enabled": true,
      "limit_max_warning": 1,
      "limit_max_error": 2
    },
    {
      "name": "CPU Diagnosepuffer",
      "address": "CPU.DiagBufCount",
      "datatype": "WORD",
      "unit": "Count",
      "limits_enabled": true,
      "limit_max_warning": 0
    },
    {
      "name": "CPU Typ",
      "address": "CPU.Info.ModuleTypeName"
    },
    {
      "name": "Tanktemperatur",
      "address": "DB5.DBD10",
      "datatype": "REAL",
      "unit": "Temperature",
      "float_decimals": 1,
      "limits_enabled": true,
      "limit_max_warning": 75.0,
      "limit_max_error": 90.0
    },
    {
      "name": "Pumpendruck",
      "address": "DB5.DBW20",
      "datatype": "INT",
      "scale_factor": 0.1,
      "unit": "Custom",
      "customunit": "bar",
      "float_decimals": 1
    },
    {
      "name": "Motor läuft",
      "address": "DB5.DBX0.0",
      "datatype": "BOOL",
      "unit": "Custom",
      "customunit": "Status"
    },
    {
      "name": "Fehlerzähler",
      "address": "MW100",
      "datatype": "WORD",
      "unit": "Count"
    },
    {
      "name": "SZL Diagnosepuffer Einträge",
      "address": "SZL.0x0074.0.2",
      "datatype": "WORD",
      "unit": "Count"
    }
  ]
}
```

---

## PRTG-Ausgabe

### Erfolg
```xml
<prtg>
  <result>
    <channel>Tanktemperatur</channel>
    <value>42.5</value>
    <float>1</float>
    <unit>Temperature</unit>
    <limitsEnabled>1</limitsEnabled>
    <limitMaxWarning>75</limitMaxWarning>
    <limitMaxError>90</limitMaxError>
  </result>
  <text>PLC RUN</text>
</prtg>
```

### Fehler
```xml
<prtg>
  <error>1</error>
  <text>S7 connect failed: 0x00050000</text>
</prtg>
```

---

## Häufige Fehler

| Fehlercode | Bedeutung | Lösung |
|---|---|---|
| `0x00C00000` | `errCliItemNotAvailable` – SZL-ID nicht vorhanden | `--list-szl` ausführen, gültige ID verwenden |
| `0x02200000` | `errCliBufferTooSmall` | Interner Pufferfehler, bitte Issue melden |
| `0x00002751` | `WSAEHOSTUNREACH` – Host nicht erreichbar | IP-Adresse, Netzwerk-Route und Firewall prüfen |
| `0x00002745` | `WSAECONNREFUSED` – Verbindung abgelehnt | SPS läuft, akzeptiert aber keine Verbindung – Rack/Slot prüfen |
| `0x00002748` | `WSAETIMEDOUT` – Timeout beim Verbinden | `timeout_ms` erhöhen, Netzwerk prüfen |
| `0x00050000` | Verbindung auf ISO-Ebene abgelehnt | PUT/GET aktivieren, Rack/Slot prüfen |
| `0x00030000` | Timeout auf ISO-Ebene | `timeout_ms` erhöhen, Netzwerk prüfen |

---

## Weiterführende Dokumentation

### snap7 / Bibliotheken

| Ressource | Link |
|---|---|
| snap7 GitHub (offizielles Repository) | [github.com/davenardella/snap7](https://github.com/davenardella/snap7) |
| snap7 Homepage & Referenzhandbuch | [snap7.sourceforge.net](https://snap7.sourceforge.net/) |
| nlohmann/json | [github.com/nlohmann/json](https://github.com/nlohmann/json) |

### Siemens: PUT/GET-Kommunikation aktivieren

| Ressource | Link |
|---|---|
| PUT/GET-Zugriff bei S7-1500 (ab FW V3.1) / S7-1200 (ab FW V4.7) aktivieren (DE) | [support.industry.siemens.com – 109925755](https://support.industry.siemens.com/cs/document/109925755/wie-aktivieren-sie-den-put-get-zugriff-bei-einer-s7-1500-cpu-ab-fw-v3-1-bzw-s7-1200-cpu-ab-fw-v4-7-) |
| How to activate PUT/GET access on S7-1500 / S7-1200 (EN) | [support.industry.siemens.com – 109925755](https://support.industry.siemens.com/cs/document/109925755/how-do-you-activate-put-get-access-on-an-s7-1500-cpu-from-fw-v3-1-or-s7-1200-cpu-from-fw-v4-7-) |

### Siemens: S7-1500 Diagnose & Diagnosepuffer

| Ressource | Link |
|---|---|
| S7-1500 Diagnosefunktionen Handbuch (PDF, EN) | [support.industry.siemens.com – 59192926](https://support.industry.siemens.com/cs/attachments/59192926/s71500_diagnosis_function_manual_en-US_en-US.pdf) |
| Diagnosepuffer-Inhalte der SIMATIC S7-CPU (EN) | [support.industry.siemens.com – 14960968](https://support.industry.siemens.com/cs/document/14960968/which-information-is-entered-in-the-diagnostic-buffer-of-the-simatic-s7-cpu-with-step-7-v5-x-) |
| SIMATIC S7-1500 / ET 200MP Manual Collection | [support.industry.siemens.com – 86140384](https://support.industry.siemens.com/cs/document/86140384/simatic-s7-1500-et-200mp-manual-collection) |

### Siemens: Systemzustandsliste (SZL)

| Ressource | Link |
|---|---|
| RDSYSST: Systemzustandsliste auslesen (S7-300/400) | [support.industry.siemens.com – 109755202](https://support.industry.siemens.com/cs/mdm/109755202?c=51087599755&lc=de-de) |
| SIMATIC System- und Standardfunktionen S7-300/400 – Kapitel SZL-IDs (PDF, DE) | [support.industry.siemens.com – 44240604](https://support.industry.siemens.com/cs/attachments/44240604/s7sfc_de-DE.pdf) |

> **Hinweis:** Siemens-Dokumentationslinks erfordern ggf. eine kostenlose Registrierung auf [support.industry.siemens.com](https://support.industry.siemens.com).

---

## Lizenz

Dieses Projekt nutzt [snap7](https://github.com/davenardella/snap7) (LGPL-3.0) und [nlohmann/json](https://github.com/nlohmann/json) (MIT).
