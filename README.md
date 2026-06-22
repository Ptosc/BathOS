# AmbientOS

Präsenzgesteuerte LED-Beleuchtung für den Schreibtisch — ESP32-Firmware für eine WS2812B-Leiste mit mmWave-Sensor, Encodern und Tagesplan.

AmbientOS steuert eine 107-LED-Streifenleiste und eine kleine Status-LED-Leiste. Licht reagiert auf Anwesenheit, lässt sich per Encoder feinjustieren und wechselt automatisch zwischen Arbeits- und Abendmodus. Drei visuelle Profile decken konzentriertes Arbeiten, dekoratives Licht und freie Farbwahl ab.

## Features

- **Präsenzerkennung** — mmWave-Radar mit asymmetrischer Glättung (schnelles Einschalten, verzögertes Ausschalten)
- **Drei Lichtmodi** — Focus, Showcase (Prism) und Canvas
- **Sanfte Übergänge** — Crossfade zwischen Modi und bei An-/Abwesenheit
- **Tagesplan** — automatischer Wechsel Focus (6–17 Uhr) / Canvas (ab 18 Uhr) via NTP, nur bei Anwesenheit
- **Helligkeitsregler** — Potentiometer mit Gamma-Korrektur und weicher Nachführung
- **Status-LEDs** — visuelles Feedback für Tasten und Shutdown

## Lichtmodi

| Modus | Beschreibung |
|-------|--------------|
| **Focus** | Kerzenflackern beim Ankommen (30 s Warm-up), danach ruhiges tiefes Blau. Enc1/Enc2 justieren je nach Phase Flacker-Geschwindigkeit, Intensität, Sättigung oder Farbton. |
| **Showcase** | Zwei gegenläufige Kometen mit Kollisionseffekt. Enc1 = Geschwindigkeit, Enc2 = Farbpalette (6 Presets). |
| **Canvas** | Gleichmäßige Volltonfarbe über die gesamte Leiste. Enc1 = Sättigung, Enc2 = Farbton (Endlos-Rad). |

## Bedienung

| Eingabe | Funktion |
|---------|----------|
| **T2** (Kurz) | Modus wechseln: Focus → Showcase → Canvas → Focus … |
| **T2** (Doppelklick) | Aus — mit roter Status-LED-Animation |
| **T1** | Focus-Warm-up überspringen (direkt in Deep Focus) |
| **T3** | In Focus: zwischen Ankunft (Kerze) und Deep Focus umschalten |
| **Encoder 1 / 2** | Modusabhängige Parameter (siehe Tabelle oben) |
| **Poti** | Gesamthelligkeit |

Beim ersten T2-Klick aus dem Aus-Zustand startet Focus. Der Tagesplan setzt den Modus nur bei Stundenwechsel oder neuer Anwesenheit — manuelles Umschalten bleibt danach möglich.

## Hardware

| Komponente | Details |
|------------|---------|
| MCU | ESP32 (Dev Module) |
| Haupt-LEDs | 107× WS2812B, GPIO 5 |
| Status-LEDs | 6× WS2812B, GPIO 33 |
| Präsenz | mmWave (UART, TX 16 / RX 34) |
| Encoder | 2× Rotary Encoder (GPIO 25/26, 13/22) |
| Taster | T1–T3, plus Encoder-Taster |
| Helligkeit | Potentiometer, GPIO 32 |

Pinbelegung und Konstanten stehen in `src/hardware.h` und `src/main.cpp`.

## Build & Flash

Voraussetzungen: [PlatformIO](https://platformio.org/)

```bash
# WiFi-Zugangsdaten anlegen (für NTP/Tagesplan)
cp secrets.ini.example secrets.ini
# secrets.ini bearbeiten — % im Passwort als %% escapen

# Bauen und flashen
pio run -t upload
```

Serial Monitor: `115200` Baud.

Ohne WiFi startet die Firmware normal; der Tagesplan bleibt dann inaktiv.

## Projektstruktur

```
src/
├── main.cpp           # Setup, Hauptschleife
├── logic.cpp          # Modi, Taster, Tagesplan, Encoder-Routing
├── modulation.cpp     # Präsenz, Helligkeit
├── schedule.cpp       # WiFi, NTP, CET/CEST
├── main_effects.cpp   # Render-Dispatch
├── focus_candle.cpp   # Kerzen-Effekt (WLED Candle Port)
├── focus_tuning.cpp   # Focus-Encoder-Parameter
├── showcase.cpp       # Prism-Kometen
├── canvas.cpp         # Vollton-Farbmodus
├── transitions.cpp    # Crossfade-Engine
├── status_effects.cpp # Status-LED
└── inputs/            # Encoder, Taster, mmWave
```

Die Architektur trennt **Input → Logic → Render**: Render-Funktionen lesen keine Sensoren; Zustandsänderungen laufen über die Logic-Schicht.

## Konfiguration

| Datei | Zweck |
|-------|--------|
| `platformio.ini` | Board, Libraries, Build-Flags |
| `secrets.ini` | WiFi SSID/Passwort (gitignored) |
| `src/hardware.h` | LED-Anzahl, Pins, Timing-Konstanten |

## Lizenz

Noch nicht festgelegt.
