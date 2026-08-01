#include "hardware.h"

static const float CANVAS_HUE_STEP = 1.2f;
static const float CANVAS_SAT_STEP = 3.0f;
static const float CANVAS_SMOOTH = 0.055f;
static const uint8_t CANVAS_VAL = 220;

static const uint8_t CANVAS_DEFAULT_HUE = 3;
static const uint8_t CANVAS_DEFAULT_SAT = 240;

static float hue = CANVAS_DEFAULT_HUE;
static float hue_target = CANVAS_DEFAULT_HUE;
static float sat = CANVAS_DEFAULT_SAT;
static float sat_target = CANVAS_DEFAULT_SAT;
static CRGB canvas_rgb = CHSV(CANVAS_DEFAULT_HUE, CANVAS_DEFAULT_SAT, CANVAS_VAL);
static bool canvas_uses_rgb_default = false;

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

static void canvas_rgb_to_hsv(CRGB rgb, float* out_h, float* out_s) {
  const uint8_t maxc = max(rgb.r, max(rgb.g, rgb.b));
  const uint8_t minc = min(rgb.r, min(rgb.g, rgb.b));
  const uint8_t delta = maxc - minc;

  if (maxc == 0 || delta == 0) {
    *out_h = 0.0f;
    *out_s = 0.0f;
    return;
  }

  *out_s = 255.0f * (float)delta / (float)maxc;

  float h;
  if (maxc == rgb.r) {
    h = 42.0f * (float)((int)rgb.g - (int)rgb.b) / (float)delta;
  } else if (maxc == rgb.g) {
    h = 42.0f + 42.0f * (float)((int)rgb.b - (int)rgb.r) / (float)delta;
  } else {
    h = 85.0f + 42.0f * (float)((int)rgb.r - (int)rgb.g) / (float)delta;
  }
  if (h < 0.0f) h += 256.0f;
  *out_h = h;
}

uint8_t canvas_hue_val() { return (uint8_t)(wrap_hue(hue_target) + 0.5f); }

void canvas_apply_defaults(uint8_t default_hue, uint8_t default_sat) {
  hue_target = (float)default_hue;
  sat_target = (float)default_sat;
  hue = hue_target;
  sat = sat_target;
  canvas_uses_rgb_default = false;
}

void canvas_apply_defaults_rgb(CRGB color) {
  canvas_rgb = color;
  canvas_rgb_to_hsv(color, &hue, &sat);
  hue_target = hue;
  sat_target = sat;
  canvas_uses_rgb_default = true;
}

void canvas_tuning_tick() {
  if (canvas_uses_rgb_default) return;
  hue = lerp_hue_toward(hue, hue_target, CANVAS_SMOOTH);
  sat = lerp_toward(sat, sat_target, CANVAS_SMOOTH);
}

void canvas_tuning_update(int16_t enc1_delta, int16_t enc2_delta) {
  if (enc1_delta != 0 || enc2_delta != 0) {
    canvas_uses_rgb_default = false;
  }
  if (enc1_delta != 0) {
    sat_target = clamp_f(sat_target + enc1_delta * CANVAS_SAT_STEP, 0.0f, 255.0f);
  }
  if (enc2_delta != 0) {
    hue_target += enc2_delta * CANVAS_HUE_STEP;
  }
}

void render_canvas(unsigned long now_ms) {
  (void)now_ms;
  if (canvas_uses_rgb_default) {
    fill_solid(leds, NUMPIXELS, canvas_rgb);
    return;
  }
  fill_solid(leds, NUMPIXELS, CHSV((uint8_t)(wrap_hue(hue) + 0.5f), (uint8_t)(sat + 0.5f), CANVAS_VAL));
}
