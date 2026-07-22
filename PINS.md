# Pin‑Map & Wiring — BathroomOS (ESP32)

## Übersicht

- Board: ESP32 (Arduino core)
- Projekt: BathroomOS — LED‑Streifen, Status‑LEDs, mmWave‑Präsenz, Taster, Rotary Encoder
- Serielle Konsole: 115200 Baud

## Pin‑Map

| GPIO | Symbol / Name           | Rolle                              | Richtung  | Hinweise |
|-----:|-------------------------|------------------------------------|-----------|----------|
| 5    | `PIN`                   | Haupt‑LED‑Strip Data (WS2812B)     | Ausgang   | 30 LEDs; FastLED |
| 33   | `STATUS_PIN`            | Status‑LED‑Strip Data (WS2812B)    | Ausgang   | 6 LEDs |
| 23   | `taster1_pin`           | Taster T1                          | Eingang   | INPUT_PULLUP; active LOW |
| 18   | `taster2_pin`           | Taster T2 (Modus)                  | Eingang   | INPUT_PULLUP; Kurz = Modus, Doppelklick = Aus, Halten = Always On |
| 17   | `taster3_pin`           | Taster T3                          | Eingang   | INPUT_PULLUP |
| 16   | `mmwave_tx_pin`         | mmWave UART TX (ESP → Modul RX)    | Ausgang   | Serial2, 115200 Baud |
| 34   | `mmwave_rx_pin`         | mmWave UART RX (ESP ← Modul TX)    | Eingang   | Nur Input; ADC2‑Pin, kein WiFi‑Konflikt bei UART |
| 25   | `encoder1_a_pin`        | Rotary Encoder 1 — A (CLK)         | Eingang   | INPUT_PULLUP; IRQ |
| 26   | `encoder1_b_pin`        | Rotary Encoder 1 — B (DT)          | Eingang   | INPUT_PULLUP; IRQ |
| 13   | `encoder2_a_pin`        | Rotary Encoder 2 — A (CLK)         | Eingang   | INPUT_PULLUP; IRQ |
| 22   | `encoder2_b_pin`        | Rotary Encoder 2 — B (DT)          | Eingang   | INPUT_PULLUP; IRQ |

### Freie GPIOs (nicht verdrahtet)

| GPIO | Hinweis |
|-----:|---------|
| 32   | Frei — z. B. für zweiten Sensor oder PIR |
| 19   | Frei — Encoder 1 Push nicht genutzt |
| 21   | Frei — Encoder 2 Push nicht genutzt |

## Taster‑Funktionen

| Taster | GPIO | Funktion |
|--------|-----:|----------|
| T1 | 23 | Visuelles Feedback (rote Status‑Zone); mit Enc1: Sättigung (Spa) bzw. Helligkeit (Showcase/Canvas) |
| T2 | 18 | Kurz: Modus wechseln (Spa → Showcase → Canvas); Doppelklick: Aus; Halten (~700 ms): Always On |
| T3 | 17 | Spa: Base ↔ Candle umschalten |

## Kurz‑Wiring

### LED‑Streifen (WS2812B)

- Haupt‑Strip (30 LEDs): DIN → GPIO **5**, 5 V + GND (gemeinsame Masse mit ESP32)
- Status‑Strip (6 LEDs): DIN → GPIO **33**, 5 V + GND

Bei 5 V‑Strips Level‑Shifter auf der Datenleitung empfohlen. 470 µF nahe der Strip‑Versorgung.

### mmWave (UART, z. B. LD2410)

- Modul VCC → 3,3 V oder 5 V (je nach Modul)
- Modul GND → ESP32 GND
- Modul **RX** → GPIO **16** (ESP TX)
- Modul **TX** → GPIO **34** (ESP RX)

Abstandswerte in **Zentimetern**. Schwellen in `hardware.h`:

- Präsenz an: raw ≤ **160 cm**
- Präsenz aus: gefiltert ≥ **210 cm**
- OFF‑Debounce: **2500 ms**
- UART‑Stale: **8000 ms** (debounced, kein sofortiges Abschalten)

### Taster

Alle Taster: eine Seite an GPIO, andere Seite an **GND** (intern Pull‑up).

| Taster | GPIO |
|--------|-----:|
| T1 | 23 |
| T2 | 18 |
| T3 | 17 |

### Rotary Encoder

| Signal | Encoder 1 | Encoder 2 |
|--------|----------:|----------:|
| A (CLK) | 25 | 13 |
| B (DT)  | 26 | 22 |
| GND     | gemeinsam | gemeinsam |

## Wichtige Hinweise

- **Gemeinsame Masse** zwischen ESP32, Sensoren, Tastern und LED‑Versorgung.
- **GPIO 34** ist reiner Eingang — nur als UART RX nutzen, nicht als Ausgang verdrahten.
- **WS2812B**: Datenpegel bei 5 V‑Strips ggf. über Level‑Shifter; Masse immer verbinden.
- **Serial Monitor**: 115200 Baud (`Serial.begin(115200)`).
- **mmWave Montage**: Seitlich/schräg montieren, nicht frontal auf Spiegel oder Glas richten.

## Relevante Dateien

| Datei | Inhalt |
|-------|--------|
| `src/main.cpp` | Pin‑Konstanten (`taster*_pin`, Encoder, mmWave) |
| `src/hardware.h` | LED‑Pins, Präsenz‑Konstanten, Modus‑Deklarationen |
| `src/inputs/mmwave_impl.cpp` | mmWave UART (Serial2, 115200) |
| `src/inputs/input_buttons.cpp` | Taster‑Initialisierung und Abfrage |
| `src/inputs/input_encoders.cpp` | Encoder IRQ und Positionszähler |
| `src/schedule.cpp` | Tagesplan‑Kurven und Nacht‑Helligkeit |
| `src/logic.cpp` | Modi, Präsenz‑Session, Fade‑In/Out |
