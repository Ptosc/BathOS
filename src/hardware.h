#pragma once
#include <Arduino.h>
#include <FastLED.h>

// LED SETUP
#define PIN 5
#define NUMPIXELS 30
#define STATUS_PIN 33
#define STATUS_LED_COUNT 6

#define COLOR_ORDER GRB
#define LED_TYPE WS2812B

// INPUT PINS
extern const int taster1_pin;
extern const int taster2_pin;
extern const int taster3_pin;
extern const int mmwave_rx_pin;
extern const int mmwave_tx_pin;
extern const int encoder1_a_pin;
extern const int encoder1_b_pin;
extern const int encoder2_a_pin;
extern const int encoder2_b_pin;

// LED BUFFERS
extern CRGB leds[NUMPIXELS];
extern CRGB status_led[STATUS_LED_COUNT];
extern CRGB prev_frame[NUMPIXELS];
extern CRGB new_frame[NUMPIXELS];
extern CRGB last_unscaled[NUMPIXELS];

// Sensor state (written by Input layer, read by Logic/Render callers)
extern int raw_distance;
extern int filtered_distance;
extern float presence;
extern float sensor_presence;

// Mode state (mode changed only via handle_mode_buttons)
extern int mode;
extern const int max_modes;
static const int MODE_OFF = 0;
static const int MODE_SPA = 1;
static const int MODE_SHOWCASE = 2;
static const int MODE_CANVAS = 3;
extern bool always_on;

enum SpaPhase {
  SPA_NONE = 0,
  SPA_BASE,
  SPA_CANDLE,
};

extern SpaPhase spa_phase;

static const unsigned long TRANSITION_MS = 800;
static const unsigned long TRANSITION_PRESENCE_MS = 280;
static const unsigned long PRESENCE_FADE_OUT_MS = 180000UL; // 3 min
static const unsigned long PRESENCE_FADE_IN_MS = 5000;      // return during fade-out

// mmWave UART presence (centimeters). Wider hysteresis + longer OFF debounce
// reduce drop-outs while occupied; stale UART is debounced like a normal OFF.
static const int MMWAVE_THRESH_ON_CM = 160;
static const int MMWAVE_THRESH_OFF_CM = 210;
static const unsigned long MMWAVE_DEBOUNCE_ON_MS = 80;
static const unsigned long MMWAVE_DEBOUNCE_OFF_MS = 2500;
static const unsigned long MMWAVE_STALE_MS = 8000;
static const float MMWAVE_FILTER_ALPHA = 0.55f;
static const float MMWAVE_FILTER_DECAY = 0.78f; // faster decay when target lost

// Logic-layer presence fusion (smooths binary sensor_presence for lighting)
static const float PRESENCE_RISE_RATE = 0.40f;
static const float PRESENCE_FALL_RATE = 0.07f;
static const float PRESENCE_ENTER_LEVEL = 0.30f;
static const float PRESENCE_EXIT_LEVEL = 0.10f;
static const unsigned long PRESENCE_EXIT_HOLD_MS = 500;

static const unsigned CANDLE_FRAME_MS = 25;
static const uint8_t CANDLE_SPEED = 96;
static const uint8_t CANDLE_INTENSITY = 224;

struct VisualState {
  int mode;
  SpaPhase spa_phase;
  bool active;
};

// Modulation (computed by Logic layer, passed into Render)
struct LightMod {
  uint8_t color_mode;
  float energy;
  uint8_t brightness;
  float motion;
};

extern LightMod mod;

// Brightness setpoint 0..255 (T1 held + encoder1 outside spa); smoothed in compute_modulation()
extern uint8_t g_brightness_raw;

// --- Input layer ---
enum ButtonEvent {
  BUTTON_NONE = 0,
  BUTTON_T1,
  BUTTON_T2,
  BUTTON_T3,
};

void init_inputs();
void poll_inputs();
ButtonEvent poll_buttons();
bool button_is_held(ButtonEvent btn);

// --- Logic layer ---
void handle_mode_buttons(ButtonEvent e);
void update_state();
bool user_is_present();
void update_spa_session();
void update_presence_session();
bool visual_is_lit();
uint8_t presence_fade_mul();
void toggle_spa_candle();
void compute_modulation();
void update_mode_button_pending();
void update_t2_hold();
void update_daynight_schedule();

void schedule_init();
bool schedule_time_valid();
int schedule_local_hour();
int schedule_local_minutes();
float schedule_local_minutes_f();
void spa_schedule_targets(float minutes, uint8_t* out_brightness, CRGB* out_color);
void spa_apply_schedule_defaults_now();

void showcase_reset();
void showcase_update(int16_t enc1_delta, int16_t enc2_delta, unsigned long now_ms);
void update_showcase_inputs();

void canvas_apply_defaults(uint8_t default_hue, uint8_t default_sat);
void canvas_tuning_tick();
void canvas_tuning_update(int16_t enc1_delta, int16_t enc2_delta);
void render_canvas(unsigned long now_ms);
uint8_t canvas_hue_val();
void update_canvas_inputs();

// --- Spa ---
void spa_apply_schedule_defaults(uint8_t default_brightness, CRGB color);
void spa_tuning_tick();
void spa_tuning_update(int16_t enc1_delta, int16_t enc2_delta, bool t1_held);
uint8_t spa_brightness_val();
uint8_t spa_sat_val();
uint8_t spa_hue_val();
CRGB spa_base_color();
void spa_schedule_tick();
void update_spa_inputs();

// --- Render layer (no input reads, no state mutation) ---
void render_off();
void render_spa(SpaPhase phase);
void render_showcase();
void render_visual_state_to(CRGB* buf, const VisualState& vs);
void spa_candle_reset();
void render_spa_candle(CRGB* out, CRGB bright, CRGB dim);
void update_status();
void trigger_zone(uint8_t zone, CRGB color);
void trigger_status_shutdown();

// --- Transitions ---
void transition_begin_if_changed(const VisualState& target, const CRGB* prev_unscaled);
bool transition_is_active();
void transition_output(CRGB* dst);
VisualState transition_get_displayed_state();
VisualState transition_get_target_state();
unsigned long transition_elapsed_ms();

// --- Encoder (position read only, IRQ-driven) ---
long get_encoder1_pos();
long get_encoder2_pos();

// --- mmWave config ---
void set_mmwave_thresholds(int on_cm, int off_cm);
void set_mmwave_debounce(unsigned long ms);
int get_mmwave_threshold_on_cm();
int get_mmwave_threshold_off_cm();
unsigned long get_mmwave_debounce_ms();

// Internal implementations (src/inputs/)
void init_encoders_impl();
void init_buttons_impl();
void init_mmwave_impl();
void read_mmwave_impl();
void poll_encoders_impl();
long get_encoder1_pos_impl();
long get_encoder2_pos_impl();
