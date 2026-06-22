# Pin‑Map & Wiring — AmbientOS (ESP32)

## Übersicht

- Board: ESP32 (Arduino core)
- Projekt: AmbientOS — LED‑Streifen, Status‑LEDs, mmWave‑Präsenz, Poti, Taster, Rotary Encoder
- Serielle Konsole: 115200 Baud

## Pin‑Map

| GPIO | Symbol / Name           | Rolle                              | Richtung  | Hinweise |
|-----:|-------------------------|------------------------------------|-----------|----------|
| 5    | `PIN`                   | Haupt‑LED‑Strip Data (WS2812B)     | Ausgang   | 107 LEDs; FastLED |
| 33   | `STATUS_PIN`            | Status‑LED‑Strip Data (WS2812B)    | Ausgang   | 6 LEDs |
| 23   | `taster1_pin`           | Taster T1                          | Eingang   | INPUT_PULLUP; active LOW |
| 18   | `taster2_pin`           | Taster T2 (Modus)                  | Eingang   | INPUT_PULLUP; Kurz = Moduswechsel, Doppelklick = Aus |
| 17   | `taster3_pin`           | Taster T3                          | Eingang   | INPUT_PULLUP |
| 19   | `taster_encoder1_pin`   | Encoder 1 — Push                   | Eingang   | INPUT_PULLUP |
| 21   | `taster_encoder2_pin`   | Encoder 2 — Push                   | Eingang   | INPUT_PULLUP |
| 16   | `mmwave_tx_pin`         | mmWave UART TX (ESP → Modul RX)    | Ausgang   | Serial2, 115200 Baud |
| 34   | `mmwave_rx_pin`         | mmWave UART RX (ESP ← Modul TX)    | Eingang   | Nur Input; ADC2‑Pin, kein WiFi‑Konflikt bei UART |
| 32   | `poti_pin`              | Potentiometer (Helligkeit)         | ADC1      | ADC1_CH4; 0..3,3 V |
| 25   | `encoder1_a_pin`        | Rotary Encoder 1 — A (CLK)         | Eingang   | INPUT_PULLUP; IRQ |
| 26   | `encoder1_b_pin`        | Rotary Encoder 1 — B (DT)          | Eingang   | INPUT_PULLUP; IRQ |
| 13   | `encoder2_a_pin`        | Rotary Encoder 2 — A (CLK)         | Eingang   | INPUT_PULLUP; IRQ |
| 22   | `encoder2_b_pin`        | Rotary Encoder 2 — B (DT)          | Eingang   | INPUT_PULLUP; IRQ |

## Taster‑Funktionen

| Taster | GPIO | Funktion |
|--------|-----:|----------|
| T1 | 23 | Focus‑Warm‑up überspringen |
| T2 | 18 | Modus wechseln (Focus → Showcase → Canvas); Doppelklick = Aus |
| T3 | 17 | In Focus: zwischen Kerze und Deep Focus umschalten |
| Enc1‑Push | 19 | — (nicht als Modus‑Taster gemappt) |
| Enc2‑Push | 21 | — (nicht als Modus‑Taster gemappt) |

## Kurz‑Wiring

### LED‑Streifen (WS2812B)

- Haupt‑Strip (107 LEDs): DIN → GPIO **5**, 5 V + GND (gemeinsame Masse mit ESP32)
- Status‑Strip (6 LEDs): DIN → GPIO **33**, 5 V + GND

Bei 5 V‑Strips Level‑Shifter auf der Datenleitung empfohlen. 470 µF nahe der Strip‑Versorgung.

### mmWave (UART, z. B. LD2410)

- Modul VCC → 3,3 V oder 5 V (je nach Modul)
- Modul GND → ESP32 GND
- Modul **RX** → GPIO **16** (ESP TX)
- Modul **TX** → GPIO **34** (ESP RX)

Abstandswerte in **Zentimetern**; Standard‑Schwellen: Präsenz an ≤ 120 cm, aus ≥ 180 cm.

### Potentiometer (z. B. 10 kΩ)

- Außenpins → 3,3 V und GND
- Mittelabgriff → GPIO **32**

### Taster

Alle Taster: eine Seite an GPIO, andere Seite an **GND** (intern Pull‑up).

| Taster | GPIO |
|--------|-----:|
| T1 | 23 |
| T2 | 18 |
| T3 | 17 |
| Encoder 1 Push | 19 |
| Encoder 2 Push | 21 |

### Rotary Encoder

| Signal | Encoder 1 | Encoder 2 |
|--------|----------:|----------:|
| A (CLK) | 25 | 13 |
| B (DT)  | 26 | 22 |
| SW      | 19 | 21 |
| GND     | gemeinsam | gemeinsam |

## Wichtige Hinweise

- **Gemeinsame Masse** zwischen ESP32, Sensoren, Tastern und LED‑Versorgung.
- **GPIO 34** ist reiner Eingang — nur als UART RX nutzen, nicht als Ausgang verdrahten.
- **ADC1** (GPIO 32) für das Poti — ADC2 kann bei aktivem WiFi stören; GPIO 34 ist für UART RX in Ordnung.
- **WS2812B**: Datenpegel bei 5 V‑Strips ggf. über Level‑Shifter; Masse immer verbinden.
- **Serial Monitor**: 115200 Baud (`Serial.begin(115200)`).

## Relevante Dateien

| Datei | Inhalt |
|-------|--------|
| `src/main.cpp` | Pin‑Konstanten (`taster*_pin`, Encoder, mmWave, Poti) |
| `src/hardware.h` | LED‑Pins (`PIN`, `STATUS_PIN`), Deklarationen |
| `src/inputs/mmwave_impl.cpp` | mmWave UART (Serial2, 115200) |
| `src/inputs/input_buttons.cpp` | Taster‑Initialisierung und Abfrage |
| `src/inputs/input_encoders.cpp` | Encoder IRQ und Positionszähler |
