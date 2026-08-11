#include "hardware.h"

static const float SPA_BRIGHTNESS_STEP = 3.5f;
static const float SPA_SAT_STEP = 3.0f;
static const float SPA_HUE_STEP = 1.2f;
static const float SPA_SMOOTH = 0.055f;
static const float SPA_SCHEDULE_SMOOTH = 0.012f;

static float brightness = 200.0f;
static float brightness_target = 200.0f;
static float hue = 32.0f;
static float hue_target = 32.0f;
static float sat = 200.0f;
static float sat_target = 200.0f;
static CRGB spa_base_rgb(255, 175, 90);
static bool spa_brightness_from_schedule = true;
static bool spa_color_from_schedule = true;

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

static CRGB lerp_rgb_toward(CRGB current, CRGB target, float rate) {
  return CRGB(
    (uint8_t)(lerp_toward((float)current.r, (float)target.r, rate) + 0.5f),
    (uint8_t)(lerp_toward((float)current.g, (float)target.g, rate) + 0.5f),
    (uint8_t)(lerp_toward((float)current.b, (float)target.b, rate) + 0.5f));
}

static float wrap_hue(float h) {
  h = fmodf(h, 256.0f);
  if (h < 0.0f) h += 256.0f;
  return h;
}

// FastLED rgb2hsv_approximate breaks on desaturated schedule colors (wrong hue/sat).
static void classic_rgb_to_hsv(CRGB rgb, float* out_h, float* out_s) {
  const uint8_t r = rgb.r;
  const uint8_t g = rgb.g;
  const uint8_t b = rgb.b;
  const uint8_t maxc = max(r, max(g, b));
  const uint8_t minc = min(r, min(g, b));
  const uint8_t delta = maxc - minc;

  if (maxc == 0 || delta == 0) {
    *out_h = 0.0f;
    *out_s = 0.0f;
    return;
  }

  *out_s = 255.0f * (float)delta / (float)maxc;

  float h;
  if (maxc == r) {
    h = 42.0f * (float)(g - b) / (float)delta;
  } else if (maxc == g) {
    h = 42.0f + 42.0f * (float)(b - r) / (float)delta;
  } else {
    h = 85.0f + 42.0f * (float)(r - g) / (float)delta;
  }
  if (h < 0.0f) h += 256.0f;
  *out_h = h;
}

uint8_t spa_brightness_val() { return (uint8_t)(brightness + 0.5f); }
uint8_t spa_sat_val() { return (uint8_t)(sat + 0.5f); }
uint8_t spa_hue_val() { return (uint8_t)(wrap_hue(hue) + 0.5f); }

CRGB spa_base_color() { return spa_base_rgb; }

void spa_apply_schedule_defaults(uint8_t default_brightness, CRGB color) {
  brightness_target = (float)default_brightness;
  brightness = brightness_target;
  spa_base_rgb = color;
  spa_brightness_from_schedule = true;
  spa_color_from_schedule = true;
  classic_rgb_to_hsv(color, &hue, &sat);
  hue_target = hue;
  sat_target = sat;
}

void spa_schedule_tick() {
  if (!schedule_time_valid()) return;

  const float minutes = schedule_local_minutes_f();
  if (minutes < 0.0f) return;

  uint8_t target_bri = 200;
  CRGB target_color = spa_base_rgb;
  spa_schedule_targets(minutes, &target_bri, &target_color);

  if (spa_color_from_schedule) {
    spa_base_rgb = lerp_rgb_toward(spa_base_rgb, target_color, SPA_SCHEDULE_SMOOTH);
    classic_rgb_to_hsv(spa_base_rgb, &hue, &sat);
    hue_target = hue;
    sat_target = sat;
  }

  if (spa_brightness_from_schedule) {
    brightness_target = lerp_toward(brightness_target, (float)target_bri, SPA_SCHEDULE_SMOOTH);
  }
}

void spa_tuning_tick() {
  brightness = lerp_toward(brightness, brightness_target, SPA_SMOOTH);
  if (spa_color_from_schedule) return;

  hue = lerp_hue_toward(hue, hue_target, SPA_SMOOTH);
  sat = lerp_toward(sat, sat_target, SPA_SMOOTH);
  spa_base_rgb = CHSV((uint8_t)(wrap_hue(hue) + 0.5f), (uint8_t)(sat + 0.5f), 255);
}

void spa_tuning_update(int16_t enc1_delta, int16_t enc2_delta, bool t2_held) {
  if (enc1_delta != 0) {
    spa_brightness_from_schedule = false;
    brightness_target = clamp_f(brightness_target + enc1_delta * SPA_BRIGHTNESS_STEP, 0.0f, 255.0f);
  }

  if (enc2_delta == 0) return;

  spa_color_from_schedule = false;
  if (t2_held) {
    sat_target = clamp_f(sat_target + enc2_delta * SPA_SAT_STEP, 0.0f, 255.0f);
  } else {
    hue_target += enc2_delta * SPA_HUE_STEP;
  }
}
