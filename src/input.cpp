#include "hardware.h"

uint8_t g_poti_raw = 0;

void init_inputs() {
  init_buttons_impl();
  init_encoders_impl();
  init_mmwave_impl();
}

void poll_inputs() {
  poll_encoders_impl();
  read_mmwave_impl();
  g_poti_raw = read_poti_impl(poti);
}

long get_encoder1_pos() { return get_encoder1_pos_impl(); }
long get_encoder2_pos() { return get_encoder2_pos_impl(); }
