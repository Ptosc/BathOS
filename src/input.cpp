#include "hardware.h"

static const uint8_t BRIGHTNESS_DEFAULT = 200;
static const int16_t BRIGHTNESS_ENC_STEP = 3;

uint8_t g_brightness_raw = BRIGHTNESS_DEFAULT;

static long last_brightness_enc1 = 0;
static bool brightness_enc_synced = false;

static void update_brightness_from_encoder() {
  if (mode == MODE_SPA) return;

  if (!button_is_held(BUTTON_T1)) {
    brightness_enc_synced = false;
    return;
  }

  const long enc1 = get_encoder1_pos();
  if (!brightness_enc_synced) {
    last_brightness_enc1 = enc1;
    brightness_enc_synced = true;
    return;
  }

  const int16_t delta = (int16_t)(enc1 - last_brightness_enc1);
  if (delta == 0) return;

  const int next = (int)g_brightness_raw + delta * BRIGHTNESS_ENC_STEP;
  g_brightness_raw = (uint8_t)constrain(next, 0, 255);
  last_brightness_enc1 = enc1;
}

void init_inputs() {
  init_buttons_impl();
  init_encoders_impl();
  init_mmwave_impl();
}

void poll_inputs() {
  poll_encoders_impl();
  read_mmwave_impl();
  update_brightness_from_encoder();
}

long get_encoder1_pos() { return get_encoder1_pos_impl(); }
long get_encoder2_pos() { return get_encoder2_pos_impl(); }
