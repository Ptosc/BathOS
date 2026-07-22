#include "../hardware.h"

static const int BUTTON_COUNT = 3;

static const int* button_pin(int idx) {
  switch (idx) {
    case 0: return &taster1_pin;
    case 1: return &taster2_pin;
    case 2: return &taster3_pin;
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
