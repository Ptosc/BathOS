#include "hardware.h"

static const float SPA_BRIGHTNESS_STEP = 3.5f;
static const float SPA_SAT_STEP = 3.0f;
static const float SPA_HUE_STEP = 1.2f;
static const float SPA_SMOOTH = 0.055f;

static float brightness = 200.0f;
static float brightness_target = 200.0f;
static float hue = 32.0f;
static float hue_target = 32.0f;
static float sat = 200.0f;
static float sat_target = 200.0f;

static float clamp_f(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static float hue_shortest_delta(float from, float to) {
  float d = to - from;
  while (d > 128.0f) d -= 256.0f;
  while (d < -128.0f) d += 256.0f;
  return d;
}

static float lerp_toward(float current, float target, float rate) {
  const float d = target - current;
  if (fabsf(d) < 0.04f) return target;
  return current + d * rate;
}

static float lerp_hue_toward(float current, float target, float rate) {
  const float d = hue_shortest_delta(current, target);
  if (fabsf(d) < 0.04f) return target;
  return current + d * rate;
}

static float wrap_hue(float h) {
  h = fmodf(h, 256.0f);
  if (h < 0.0f) h += 256.0f;
  return h;
}

uint8_t spa_brightness_val() { return (uint8_t)(brightness + 0.5f); }
uint8_t spa_sat_val() { return (uint8_t)(sat + 0.5f); }
uint8_t spa_hue_val() { return (uint8_t)(wrap_hue(hue) + 0.5f); }

void spa_apply_schedule_defaults(uint8_t default_brightness, CRGB color) {
  brightness_target = (float)default_brightness;
  brightness = brightness_target;

  const CHSV hsv = rgb2hsv_approximate(color);
  hue_target = (float)hsv.hue;
  hue = hue_target;
  sat_target = (float)hsv.sat;
  sat = sat_target;
}

void spa_tuning_tick() {
  brightness = lerp_toward(brightness, brightness_target, SPA_SMOOTH);
  hue = lerp_hue_toward(hue, hue_target, SPA_SMOOTH);
  sat = lerp_toward(sat, sat_target, SPA_SMOOTH);
}

void spa_tuning_update(int16_t enc1_delta, int16_t enc2_delta, bool t1_held) {
  if (t1_held && enc1_delta != 0) {
    sat_target = clamp_f(sat_target + enc1_delta * SPA_SAT_STEP, 0.0f, 255.0f);
  } else if (enc1_delta != 0) {
    brightness_target = clamp_f(brightness_target + enc1_delta * SPA_BRIGHTNESS_STEP, 0.0f, 255.0f);
  }

  if (enc2_delta != 0) {
    hue_target += enc2_delta * SPA_HUE_STEP;
  }
}
