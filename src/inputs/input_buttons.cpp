#include "../hardware.h"

static const int BUTTON_COUNT = 5;

static const int* button_pin(int idx) {
  switch (idx) {
    case 0: return &taster1_pin;
    case 1: return &taster2_pin;
    case 2: return &taster3_pin;
    case 3: return &taster_encoder1_pin;
    case 4: return &taster_encoder2_pin;
    default: return &taster1_pin;
  }
}

static bool is_pressed(int idx) {
  return digitalRead(*button_pin(idx)) == LOW;
}

void init_buttons_impl() {
  pinMode(taster1_pin, INPUT_PULLUP);
  pinMode(taster2_pin, INPUT_PULLUP);
  pinMode(taster3_pin, INPUT_PULLUP);
  pinMode(taster_encoder1_pin, INPUT_PULLUP);
  pinMode(taster_encoder2_pin, INPUT_PULLUP);
  pinMode(poti_pin, INPUT);
}

// Single debounce: rising edge (released -> pressed), one event per press.
static bool debounce_rising_edge(int idx) {
  static bool was_pressed[BUTTON_COUNT] = {};
  static unsigned long last_event_ms[BUTTON_COUNT] = {};
  const unsigned long DEBOUNCE_MS = 50;

  bool pressed = is_pressed(idx);
  unsigned long now = millis();
  bool fired = false;

  if (pressed && !was_pressed[idx] && (now - last_event_ms[idx]) >= DEBOUNCE_MS) {
    last_event_ms[idx] = now;
    fired = true;
  }

  was_pressed[idx] = pressed;
  return fired;
}

ButtonEvent poll_buttons() {
  if (debounce_rising_edge(0)) return BUTTON_T1;
  if (debounce_rising_edge(1)) return BUTTON_T2;
  if (debounce_rising_edge(2)) return BUTTON_T3;
  return BUTTON_NONE;
}

bool button_is_held(ButtonEvent btn) {
  switch (btn) {
    case BUTTON_T1: return is_pressed(0);
    case BUTTON_T2: return is_pressed(1);
    case BUTTON_T3: return is_pressed(2);
    default: return false;
  }
}

uint8_t read_poti_impl(Poti& p) {
  const float POTI_SMOOTH_FACTOR = 0.1f;
  long sum = 0;
  for (int i = 0; i < 4; i++) sum += analogRead(p.pin);
  int raw = sum / 4;
  p.smooth += (raw - p.smooth) * POTI_SMOOTH_FACTOR;
  return constrain(map((int)p.smooth, 0, 4095, 0, 255), 0, 255);
}
