#include "hardware.h"

void render_off() {
  fill_solid(leds, NUMPIXELS, CRGB::Black);
}

void render_visual_state_to(CRGB* buf, const VisualState& vs) {
  if (!vs.active || vs.mode == MODE_OFF) {
    fill_solid(buf, NUMPIXELS, CRGB::Black);
    return;
  }

  // Stateful effects must continue from their own target frame while a
  // transition is being blended into the displayed LED buffer.
  if (buf != leds) {
    for (int i = 0; i < NUMPIXELS; i++) leds[i] = buf[i];
  }

  switch (vs.mode) {
    case MODE_SPA:
      render_spa(vs.spa_phase);
      break;
    case MODE_SHOWCASE:
      render_showcase();
      break;
    case MODE_CANVAS:
      render_canvas(millis());
      break;
    default:
      render_off();
      break;
  }

  for (int i = 0; i < NUMPIXELS; i++) buf[i] = leds[i];
}
