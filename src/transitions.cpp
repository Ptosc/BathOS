#include "hardware.h"

static VisualState displayed_state = {MODE_OFF, FOCUS_NONE, false};
static VisualState transition_target = {MODE_OFF, FOCUS_NONE, false};
static unsigned long transition_start_ms = 0;
static unsigned long transition_duration_ms = TRANSITION_MS;
static bool transitioning = false;

static bool visual_state_equals(const VisualState& a, const VisualState& b) {
  return a.mode == b.mode
      && a.focus_phase == b.focus_phase
      && a.active == b.active;
}

static unsigned long transition_ms_for(const VisualState& from, const VisualState& to) {
  if (from.mode == to.mode && from.focus_phase == to.focus_phase && from.active != to.active) {
    return TRANSITION_PRESENCE_MS;
  }
  return TRANSITION_MS;
}

static float ease_in_out(float t) {
  return t * t * (3.0f - 2.0f * t);
}

static void blend_frames_to(CRGB* a, CRGB* b, CRGB* dst, float t) {
  float te = ease_in_out(constrain(t, 0.0f, 1.0f));
  uint8_t ta = (uint8_t)((1.0f - te) * 255);
  uint8_t tb = (uint8_t)(te * 255);
  for (int i = 0; i < NUMPIXELS; i++) {
    CRGB aa = a[i];
    CRGB bb = b[i];
    uint16_t r = (uint16_t)scale8_video(aa.r, ta) + (uint16_t)scale8_video(bb.r, tb);
    uint16_t g = (uint16_t)scale8_video(aa.g, ta) + (uint16_t)scale8_video(bb.g, tb);
    uint16_t b_ = (uint16_t)scale8_video(aa.b, ta) + (uint16_t)scale8_video(bb.b, tb);
    dst[i].r = (r > 255) ? 255 : (uint8_t)r;
    dst[i].g = (g > 255) ? 255 : (uint8_t)g;
    dst[i].b = (b_ > 255) ? 255 : (uint8_t)b_;
  }
}

static void capture_frame(CRGB* dst, const CRGB* src) {
  for (int i = 0; i < NUMPIXELS; i++) dst[i] = src[i];
}

void transition_begin_if_changed(const VisualState& target, const CRGB* prev_unscaled) {
  if (transitioning) {
    if (!visual_state_equals(transition_target, target)) {
      capture_frame(prev_frame, prev_unscaled);
      transition_target = target;
      transition_duration_ms = transition_ms_for(displayed_state, target);
      transition_start_ms = millis();
    }
    return;
  }

  if (visual_state_equals(displayed_state, target)) return;

  capture_frame(prev_frame, prev_unscaled);
  transition_target = target;
  transition_duration_ms = transition_ms_for(displayed_state, target);
  transition_start_ms = millis();
  transitioning = true;
}

bool transition_is_active() {
  return transitioning;
}

void transition_output(CRGB* dst) {
  if (!transitioning) return;

  float t = (float)(millis() - transition_start_ms) / (float)transition_duration_ms;
  if (t >= 1.0f) {
    transitioning = false;
    displayed_state = transition_target;
    capture_frame(dst, new_frame);
    return;
  }

  blend_frames_to(prev_frame, new_frame, dst, t);
}

VisualState transition_get_displayed_state() {
  return displayed_state;
}

VisualState transition_get_target_state() {
  return transition_target;
}

unsigned long transition_elapsed_ms() {
  if (!transitioning) return transition_duration_ms;
  unsigned long elapsed = millis() - transition_start_ms;
  return (elapsed > transition_duration_ms) ? transition_duration_ms : elapsed;
}
