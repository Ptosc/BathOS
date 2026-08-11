#include "hardware.h"
#include <Arduino.h>
#include <FastLED.h>

// ===== PIN DEFINITIONS =====

const int taster1_pin = 23;  
const int taster2_pin = 18;
const int taster3_pin = 17;

const int mmwave_tx_pin = 16;
const int mmwave_rx_pin = 34;

const int encoder1_a_pin = 25;
const int encoder1_b_pin = 26;
const int encoder2_a_pin = 13;
const int encoder2_b_pin = 22;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("BOOT");

  FastLED.addLeds<WS2812B, PIN, COLOR_ORDER>(leds, NUMPIXELS);
  FastLED.addLeds<WS2812B, STATUS_PIN, COLOR_ORDER>(status_led, STATUS_LED_COUNT);
  FastLED.setBrightness(255);
  FastLED.clear(true);

  init_inputs();

  schedule_init();
}

static void apply_brightness(CRGB* frame, uint8_t brightness) {
  if (brightness >= 255) return;
  for (int i = 0; i < NUMPIXELS; i++) {
    frame[i].nscale8_video(brightness);
  }
}

static void apply_output_scaling(CRGB* frame) {
  apply_brightness(frame, mod.brightness);

  const uint8_t fade_mul = presence_fade_mul();
  if (fade_mul >= 255) return;
  for (int i = 0; i < NUMPIXELS; i++) {
    frame[i].nscale8_video(fade_mul);
  }
}

void loop() {
  poll_inputs();
  handle_mode_buttons(poll_buttons());
  update_t2_hold();
  update_mode_button_pending();

  update_state();
  update_presence_session();
  update_daynight_schedule();
  if (mode == MODE_SPA) {
    spa_schedule_tick();
  }
  compute_modulation();

  update_canvas_inputs();
  update_rainbow_inputs();
  update_spa_inputs();

  update_status();

  const VisualState target = {mode, spa_phase, visual_is_lit()};

  transition_begin_if_changed(target, last_displayed);

  const bool was_transitioning = transition_is_active();

  if (was_transitioning) {
    render_visual_state_to(new_frame, target);
    apply_output_scaling(new_frame);
    transition_output(leds);
  } else {
    render_visual_state_to(leds, target);
    apply_output_scaling(leds);
  }

  for (int i = 0; i < NUMPIXELS; i++) {
    last_displayed[i] = leds[i];
  }

  FastLED.show();
  poll_encoders_impl();
  delay(1);
  yield();
}
