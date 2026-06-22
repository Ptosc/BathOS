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

uint8_t canvas_hue_val() { return (uint8_t)(wrap_hue(hue_target) + 0.5f); }

void canvas_apply_defaults(uint8_t default_hue, uint8_t default_sat) {
  hue_target = (float)default_hue;
  sat_target = (float)default_sat;
  hue = hue_target;
  sat = sat_target;
}

void canvas_tuning_tick() {
  hue = lerp_hue_toward(hue, hue_target, CANVAS_SMOOTH);
  sat = lerp_toward(sat, sat_target, CANVAS_SMOOTH);
}

void canvas_tuning_update(int16_t enc1_delta, int16_t enc2_delta) {
  if (enc1_delta != 0) {
    sat_target = clamp_f(sat_target + enc1_delta * CANVAS_SAT_STEP, 0.0f, 255.0f);
  }
  if (enc2_delta != 0) {
    hue_target += enc2_delta * CANVAS_HUE_STEP;
  }
}

void render_canvas(unsigned long now_ms) {
  (void)now_ms;
  fill_solid(leds, NUMPIXELS, CHSV((uint8_t)(wrap_hue(hue) + 0.5f), (uint8_t)(sat + 0.5f), CANVAS_VAL));
}
