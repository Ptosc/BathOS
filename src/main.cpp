#include "hardware.h"
#include <Arduino.h>
#include <FastLED.h>

// ===== PIN DEFINITIONS =====

const int taster1_pin = 23;
const int taster2_pin = 18;
const int taster3_pin = 17;
const int taster_encoder1_pin = 19;
const int taster_encoder2_pin = 21;

const int mmwave_tx_pin = 16;
const int mmwave_rx_pin = 34;
const int poti_pin = 32;

const int encoder1_a_pin = 25;
const int encoder1_b_pin = 26;
const int encoder2_a_pin = 13;
const int encoder2_b_pin = 22;

void setup() {
  FastLED.addLeds<WS2812B, PIN, GRB>(leds, NUMPIXELS);
  FastLED.addLeds<WS2812B, STATUS_PIN, GRB>(status_led, STATUS_LED_COUNT);
  FastLED.setBrightness(255);
  FastLED.clear(true);

  init_inputs();

  Serial.begin(115200);
  Serial.println("BOOT");

  schedule_init();
}

static void apply_brightness(uint8_t brightness) {
  if (brightness >= 255) return;
  for (int i = 0; i < NUMPIXELS; i++) {
    leds[i].nscale8_video(brightness);
  }
}

void loop() {
  poll_inputs();
  handle_mode_buttons(poll_buttons());
  update_mode_button_pending();

  update_state();
  update_focus_session();
  update_daynight_schedule();
  compute_modulation();

  const bool active = user_is_present();
  update_showcase_inputs();
  update_canvas_inputs();
  update_focus_inputs();

  update_status();

  const VisualState target = {mode, focus_phase, active};

  transition_begin_if_changed(target, last_unscaled);

  const bool was_transitioning = transition_is_active();

  if (was_transitioning) {
    render_visual_state_to(new_frame, target);
    transition_output(leds);
  } else {
    render_visual_state_to(leds, target);
  }

  for (int i = 0; i < NUMPIXELS; i++) {
    last_unscaled[i] = leds[i];
  }

  apply_brightness(mod.brightness);

  FastLED.show();
  poll_encoders_impl();
  delay(1);
  yield();
}
