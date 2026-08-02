# BathroomOS

Präsenzgesteuerte LED-Beleuchtung fürs Bad — ESP32-Firmware für eine WS2812B-Leiste mit mmWave-Sensor, zwei Rotary-Encodern und einem zeitbasierten Tagesplan.

BathroomOS schaltet das Licht bei Anwesenheit ein, dimmt es langsam aus, wenn niemand mehr da ist, und passt Farbe und Helligkeit über den Tag an. Canvas und Rainbow bieten freie Farbwahl bzw. dekorativen Verlauf.

## Features

- **Präsenzerkennung** — mmWave-Radar (LD2410 o. Ä.) mit asymmetrischer Glättung und Hysterese
- **Spa-Modus** — Hauptmodus mit warmem Tagesplan und Encoder-Feintuning
- **Sanfter Tagesplan** — Farbe und Helligkeit folgen der Uhrzeit kontinuierlich (NTP, CET/CEST)
- **Langsames Ausklingen** — nach Verlassen des Raums dimmt das Licht in allen aktiven Modi über **3 Minuten** aus
- **Sanftes Wieder-Ein** — Rückkehr während des Fade-Outs: Helligkeit fährt über **5 Sekunden** wieder hoch
- **Always On** — Präsenzerkennung umgehen (T2 halten)
- **Sanfte Übergänge** — Crossfade zwischen Modi und bei Anwesenheit
- **Status-LEDs** — visuelles Feedback für Tasten, Shutdown und Always On

## Lichtmodi

| Modus | Beschreibung |
|-------|--------------|
| **Spa** | Warme Volltonbeleuchtung nach Tagesplan. Enc1 = Helligkeit, Enc2 = Hue, T2+Enc2 = Sättigung. |
| **Canvas** | Gleichmäßige Volltonfarbe. Enc1 = Helligkeit, Enc2 = Hue, T2+Enc2 = Sättigung. |
| **Rainbow** | Dekorativer Verlauf. Enc1 = Helligkeit, Enc2 = Preset. |

### Spa-Phasen

| Phase | Beschreibung |
|-------|--------------|
| **Spa Base** | Einheitliche Farbe aus dem Tagesplan, direkt als RGB gerendert |
| **Spa Candle** | WLED-inspirierter Kerzen-Effekt (nur noch über Defaults/Auto, kein T3-Toggle) |

## Bedienung

| Eingabe | Funktion |
|---------|----------|
| **T1** | Vorheriger Modus (Spa ← Canvas ← Rainbow) |
| **T3** | Nächster Modus; aus OFF: letzter Modus / Spa |
| **T2** (tippen) | An / Aus |
| **T2** (halten ~700 ms) | Always On ein/aus — Status-LED leuchtet dauerhaft grün |
| **Enc1** | Helligkeit — Status: warmer weißer Komet |
| **Enc2** | Spa / Canvas: Hue · Rainbow: Preset — Status: farbiger Komet |
| **T2 + Enc2** | Spa / Canvas: Sättigung — Status: Magenta-Komet |

Beim ersten Betreten des Raums (oder aus dem Aus-Zustand) startet **Spa**.

### Manuelle Anpassungen (Spa)

| Einstellung | Verhalten |
|-------------|-----------|
| **Enc1 (Helligkeit)** | Überschreibt den Tagesplan für die **aktuelle Präsenz-Session** — auch bei kurzem Rausgehen und Zurückkommen während des Fade-Outs |
| **Enc2 / T2+Enc2 (Farbe)** | Überschreibt den Tagesplan bis das Licht **vollständig aus** ist (nach 3 min Fade) |
| **Nächste Session** | Nach vollständigem Auto-Off werden Tagesplan-Helligkeit und -Farbe neu gesetzt |

## Präsenz & Fade

| Situation | Verhalten |
|-----------|-----------|
| Raum betreten | Licht geht an (Spa startet aus Aus-Modus) |
| Raum verlassen | 3 Minuten Fade-Out in Spa, Canvas und Rainbow |
| Während Fade-Out zurück | Sanftes Hochfahren über 5 Sekunden — manuelle Helligkeit bleibt erhalten |
| Vollständig aus | Nächster Besuch startet mit frischem Tagesplan |
| Always On | Präsenz wird simuliert — kein Fade-Out |

## Tagesplan (Spa)

Der Plan läuft nur mit gültiger NTP-Zeit (WiFi erforderlich). Farbe und Helligkeit werden fortlaufend angeglichen — kein harter Sprung zur vollen Stunde.

| Uhrzeit | Verhalten |
|---------|-----------|
| 05:00–08:00 | Aufwachen: sanfter Übergang von Nacht-minimal zu Tageslicht |
| 08:00–16:00 | Tag: volle Helligkeit, warmes Tageslicht |
| 16:00–21:00 | Abend: zunehmend wärmer und gedimmter |
| 21:00–22:00 | Übergang auf Nacht-minimal |
| 22:00–05:00 | Nacht: minimale Helligkeit (Orientierungslicht) |

Ankerfarben (RGB): Morgen `(255,215,170)` · Tag `(255,175,90)` · Abend `(255,85,12)` · Nacht `(255,50,5)`

Nacht-Helligkeit: **10/255** (konfigurierbar als `NIGHT_MIN_BRIGHTNESS` in `schedule.cpp`)

## Hardware

| Komponente | Details |
|------------|---------|
| MCU | ESP32 (Dev Module) |
| Haupt-LEDs | 30× WS2812B, GPIO 5, Farbreihenfolge **GRB** |
| Status-LEDs | 6× WS2812B, GPIO 33 |
| Präsenz | mmWave UART — TX GPIO 16, RX GPIO 34 |
| Encoder 1 | A/B GPIO 25/26 |
| Encoder 2 | A/B GPIO 13/22 |
| Taster | T1 GPIO 23, T2 GPIO 18, T3 GPIO 17 |

Ausführliche Pinbelegung: [`PINS.md`](PINS.md)

## Build & Flash

Voraussetzungen: [PlatformIO](https://platformio.org/)

```bash
# WiFi-Zugangsdaten für NTP/Tagesplan
cp secrets.ini.example secrets.ini
# secrets.ini bearbeiten — % im Passwort als %% escapen

# Bauen und flashen
pio run -t upload

# Serial Monitor
pio device monitor
```

Serial Monitor: **115200** Baud.

Ohne WiFi startet die Firmware normal; der Tagesplan bleibt dann inaktiv (Fallback-Farbe).

### Debug-Flags

In `platformio.ini` unter `build_flags` (für Entwicklung, in Produktion entfernen):

| Flag | Ausgabe |
|------|---------|
| `SCHEDULE_DEBUG` | `[SCH]` — NTP, angewandte Farbe/Helligkeit |
| `MMWAVE_DEBUG` | `[MMW]` — Präsenz und Distanz |

## Projektstruktur

```
src/
├── main.cpp              # Setup, Hauptschleife, Fade-Out
├── logic.cpp             # Modi, Taster, Präsenz-Session, Tagesplan-Trigger
├── schedule.cpp          # WiFi, NTP, CET/CEST, Tageskurven
├── spa.cpp               # Spa-Rendering (Base + Candle)
├── spa_tuning.cpp        # Encoder, Schedule-Lerp, RGB-Farbquelle
├── spa_candle.cpp        # Kerzen-Effekt (WLED-Port)
├── showcase.cpp          # Kometen-Effekt
├── canvas.cpp            # Vollton-Farbmodus
├── modulation.cpp        # Präsenz-Glättung
├── transitions.cpp       # Crossfade-Engine
├── status_effects.cpp    # Status-LED
├── input.cpp             # Input-Orchestrierung
└── inputs/               # Encoder, Taster, mmWave
```

Architektur: **Input → Logic → Render**. Render-Funktionen lesen keine Sensoren; Zustandsänderungen laufen über die Logic-Schicht.

## Konfiguration

| Datei | Zweck |
|-------|--------|
| `platformio.ini` | Board, Libraries, Build-Flags, Upload-Port |
| `secrets.ini` | WiFi SSID/Passwort (gitignored) |
| `src/hardware.h` | LED-Anzahl, Timing, Präsenz-Schwellen, Modus-Konstanten |
| `src/schedule.cpp` | Tagesplan-Kurven, Nacht-Helligkeit |

Wichtige Konstanten in `hardware.h`:

- `NUMPIXELS` — Anzahl Haupt-LEDs (aktuell 30)
- `PRESENCE_FADE_OUT_MS` — Ausklingzeit nach Verlassen (180000 = 3 min)
- `PRESENCE_FADE_IN_MS` — Hochfahren bei Rückkehr während Fade (5000 = 5 s)
- `MMWAVE_THRESH_ON_CM` / `MMWAVE_THRESH_OFF_CM` — Präsenz-Schwellen (160/210 cm)
- `COLOR_ORDER` — LED-Farbkanal-Reihenfolge (`GRB` für WS2812B)

## Lizenz

Noch nicht festgelegt.
