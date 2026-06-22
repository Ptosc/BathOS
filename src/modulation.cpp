#include "hardware.h"

static float intensity_smooth = 0.0f;

void update_state() {
  if (sensor_presence > presence) {
    presence = presence * 0.4f + sensor_presence * 0.6f;
  } else {
    presence = presence * 0.88f + sensor_presence * 0.12f;
  }
}

bool user_is_present() {
  return always_on || sensor_presence > 0.5f || presence > 0.02f;
}

void compute_modulation() {
  const float GAMMA = 2.2f;
  const float ALPHA = 0.08f;
  const float MAX_DELTA = 8.0f;

  float x = (float)g_poti_raw / 255.0f;
  float mapped = powf(x, GAMMA) * 255.0f;

  intensity_smooth += (mapped - intensity_smooth) * ALPHA;

  float current = (float)mod.brightness;
  float delta = intensity_smooth - current;
  if (delta > MAX_DELTA) delta = MAX_DELTA;
  if (delta < -MAX_DELTA) delta = -MAX_DELTA;

  float next = current + delta;
  mod.brightness = (uint8_t)constrain((int)(next + 0.5f), 0, 255);

  mod.energy = presence;
  mod.motion = presence * 0.8f;
}
