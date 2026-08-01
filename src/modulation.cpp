#include "hardware.h"

static float intensity_smooth = 0.0f;
static bool user_occupied = false;
static unsigned long absent_hold_start_ms = 0;

static float map_brightness(uint8_t raw) {
  const float GAMMA = 2.2f;
  const float x = (float)raw / 255.0f;
  return powf(x, GAMMA) * 255.0f;
}

static void update_presence_signal() {
  if (sensor_presence > 0.5f) {
    presence += (1.0f - presence) * PRESENCE_RISE_RATE;
    absent_hold_start_ms = 0;
    return;
  }

  presence *= (1.0f - PRESENCE_FALL_RATE);
}

static void update_occupancy_latch() {
  const bool strong_signal = sensor_presence > 0.5f || presence >= PRESENCE_ENTER_LEVEL;
  const bool weak_signal = presence <= PRESENCE_EXIT_LEVEL && sensor_presence < 0.5f;

  if (!user_occupied) {
    if (strong_signal) user_occupied = true;
    absent_hold_start_ms = 0;
    return;
  }

  if (strong_signal) {
    absent_hold_start_ms = 0;
    return;
  }

  if (!weak_signal) {
    absent_hold_start_ms = 0;
    return;
  }

  const unsigned long now = millis();
  if (absent_hold_start_ms == 0) {
    absent_hold_start_ms = now;
    return;
  }
  if ((now - absent_hold_start_ms) >= PRESENCE_EXIT_HOLD_MS) {
    user_occupied = false;
    absent_hold_start_ms = 0;
  }
}

void update_state() {
  update_presence_signal();
  update_occupancy_latch();
}

bool user_is_present() {
  return always_on || user_occupied;
}

void set_global_brightness_immediate(uint8_t raw) {
  g_brightness_raw = raw;
  intensity_smooth = map_brightness(raw);
  mod.brightness = (uint8_t)constrain((int)(intensity_smooth + 0.5f), 0, 255);
}

void compute_modulation() {
  if (mode == MODE_SPA) {
    mod.brightness = 255;
    mod.energy = presence;
    mod.motion = presence * 0.8f;
    return;
  }

  const float ALPHA = 0.08f;
  const float MAX_DELTA = 8.0f;

  const float mapped = map_brightness(g_brightness_raw);

  intensity_smooth += (mapped - intensity_smooth) * ALPHA;

  float current = (float)mod.brightness;
  float delta = intensity_smooth - current;
  if (delta > MAX_DELTA) delta = MAX_DELTA;
  if (delta < -MAX_DELTA) delta = -MAX_DELTA;

  const float next = current + delta;
  mod.brightness = (uint8_t)constrain((int)(next + 0.5f), 0, 255);

  mod.energy = presence;
  mod.motion = presence * 0.8f;
}
