#include "../hardware.h"
#include <Arduino.h>

// mmWave UART reader (LD2410 and similar text/binary line emitters).
// Distances are normalized to centimeters before thresholding.

static const int MMWAVE_BAUD = 115200;
static const int MMWAVE_RX_PIN = 34;
static const int MMWAVE_TX_PIN = 16;

static int _mmw_thresh_on = MMWAVE_THRESH_ON_CM;
static int _mmw_thresh_off = MMWAVE_THRESH_OFF_CM;
static unsigned long _mmw_debounce_on_ms = MMWAVE_DEBOUNCE_ON_MS;
static unsigned long _mmw_debounce_off_ms = MMWAVE_DEBOUNCE_OFF_MS;

static HardwareSerial mmSerial(2);

static char line_buf[128];
static size_t line_idx = 0;

static int raw_range_cm = -1;
static float filtered_range_cm = 0.0f;
static bool mm_active = false;
static unsigned long mm_on_candidate_since = 0;
static unsigned long mm_off_candidate_since = 0;
static unsigned long last_frame_ms = 0;

#ifdef MMWAVE_DEBUG
static bool _last_mm_active = false;
static bool _last_stale = false;
#endif

static void note_frame() {
  last_frame_ms = millis();
}

static void invalidate_range() {
  raw_range_cm = -1;
}

static void decay_filter_toward_off() {
  const float off_anchor = (float)(_mmw_thresh_off + 20);
  if (filtered_range_cm <= 0.0f) {
    filtered_range_cm = off_anchor;
    return;
  }
  filtered_range_cm = filtered_range_cm * MMWAVE_FILTER_DECAY
                    + off_anchor * (1.0f - MMWAVE_FILTER_DECAY);
}

static void process_line(const char* s) {
  const char* p = s;
  while (*p && !((*p >= '0' && *p <= '9') || *p == '-')) p++;
  if (!*p) return;

  note_frame();

  const long v = atol(p);
  if (v == 0) {
    invalidate_range();
    decay_filter_toward_off();
    return;
  }

  const int cm = (v > 500) ? (int)((v + 5) / 10) : (int)v;
  if (cm <= 0) {
    invalidate_range();
    decay_filter_toward_off();
    return;
  }

  raw_range_cm = cm;
  if (filtered_range_cm <= 0.0f) filtered_range_cm = (float)raw_range_cm;
  filtered_range_cm = (MMWAVE_FILTER_ALPHA * raw_range_cm)
                    + ((1.0f - MMWAVE_FILTER_ALPHA) * filtered_range_cm);
}

static bool reading_is_stale(unsigned long now) {
  return (last_frame_ms > 0) && ((now - last_frame_ms) > MMWAVE_STALE_MS);
}

static bool wants_presence_on(bool stale) {
  if (stale) return false;
  if (raw_range_cm <= 0) return false;
  return raw_range_cm <= _mmw_thresh_on;
}

static bool wants_presence_off(bool stale) {
  if (stale) return true;
  if (raw_range_cm <= 0) return true;
  return filtered_range_cm >= (float)_mmw_thresh_off;
}

static void update_mm_active(unsigned long now, bool want_on, bool want_off) {
  if (!mm_active) {
    if (!want_on) {
      mm_on_candidate_since = 0;
      return;
    }
    if (mm_on_candidate_since == 0) mm_on_candidate_since = now;
    if ((now - mm_on_candidate_since) < _mmw_debounce_on_ms) return;

    mm_active = true;
    mm_on_candidate_since = 0;
    mm_off_candidate_since = 0;
    return;
  }

  if (!want_off) {
    mm_off_candidate_since = 0;
    return;
  }
  if (mm_off_candidate_since == 0) mm_off_candidate_since = now;
  if ((now - mm_off_candidate_since) < _mmw_debounce_off_ms) return;

  mm_active = false;
  mm_on_candidate_since = 0;
  mm_off_candidate_since = 0;
}

void set_mmwave_thresholds(int on_cm, int off_cm) {
  if (on_cm > 0 && off_cm > on_cm) {
    _mmw_thresh_on = on_cm;
    _mmw_thresh_off = off_cm;
  }
#ifdef MMWAVE_DEBUG
  Serial.printf("[MMW] thresholds: on=%dcm off=%dcm\n", _mmw_thresh_on, _mmw_thresh_off);
#endif
}

void set_mmwave_debounce(unsigned long ms) {
  if (ms >= 100) _mmw_debounce_off_ms = ms;
#ifdef MMWAVE_DEBUG
  Serial.printf("[MMW] debounce off: %lums\n", _mmw_debounce_off_ms);
#endif
}

int get_mmwave_threshold_on_cm() { return _mmw_thresh_on; }
int get_mmwave_threshold_off_cm() { return _mmw_thresh_off; }
unsigned long get_mmwave_debounce_ms() { return _mmw_debounce_off_ms; }

void init_mmwave_impl() {
  mmSerial.begin(MMWAVE_BAUD, SERIAL_8N1, MMWAVE_RX_PIN, MMWAVE_TX_PIN);
#ifdef MMWAVE_DEBUG
  Serial.printf("[MMW] UART started on=%d off=%d deb_on=%lums deb_off=%lums stale=%lums\n",
                _mmw_thresh_on, _mmw_thresh_off,
                _mmw_debounce_on_ms, _mmw_debounce_off_ms, MMWAVE_STALE_MS);
#endif
}

void read_mmwave_impl() {
  while (mmSerial.available()) {
    const char c = (char)mmSerial.read();
    if (c == '\r') continue;
    if (c == '\n' || line_idx >= sizeof(line_buf) - 1) {
      if (line_idx > 0) {
        line_buf[line_idx] = '\0';
        process_line(line_buf);
      }
      line_idx = 0;
    } else if (isPrintable(c)) {
      line_buf[line_idx++] = c;
    }
  }

  const unsigned long now = millis();
  const bool stale = reading_is_stale(now);

  if (stale) {
    invalidate_range();
    decay_filter_toward_off();
  }

  update_mm_active(now, wants_presence_on(stale), wants_presence_off(stale));

  raw_distance = raw_range_cm;
  filtered_distance = (raw_range_cm > 0) ? (int)(filtered_range_cm + 0.5f) : -1;
  sensor_presence = mm_active ? 1.0f : 0.0f;

#ifdef MMWAVE_DEBUG
  static unsigned long last_status_ms = 0;
  if (now - last_status_ms >= 1000) {
    last_status_ms = now;
    if (raw_range_cm > 0) {
      Serial.printf("[MMW] active=%s raw=%dcm filt=%.0fcm%s\n",
                    mm_active ? "yes" : "no", raw_range_cm, filtered_range_cm,
                    stale ? " STALE" : "");
    } else {
      Serial.printf("[MMW] active=%s raw=--- filt=%.0fcm%s off=%lums\n",
                    mm_active ? "yes" : "no", filtered_range_cm,
                    stale ? " STALE" : "",
                    mm_off_candidate_since ? (now - mm_off_candidate_since) : 0UL);
    }
  }

  if (stale != _last_stale) {
    _last_stale = stale;
    if (stale) Serial.println("[MMW] >>> UART stale — debounced OFF pending");
    else Serial.println("[MMW] >>> UART frames resumed");
  }

  if (mm_active != _last_mm_active) {
    _last_mm_active = mm_active;
    Serial.printf("[MMW] >>> sensor presence %s\n", mm_active ? "ON" : "OFF");
  }
#endif
}
