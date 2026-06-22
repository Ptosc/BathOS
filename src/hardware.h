#pragma once
#include <Arduino.h>
#include <FastLED.h>

// LED SETUP
#define PIN 5
#define NUMPIXELS 107
#define STATUS_PIN 33
#define STATUS_LED_COUNT 6

#define COLOR_ORDER GRB
#define LED_TYPE WS2812B

// INPUT PINS
extern const int taster1_pin;
extern const int taster2_pin;
extern const int taster3_pin;
extern const int taster_encoder1_pin;
extern const int taster_encoder2_pin;
extern const int mmwave_rx_pin;
extern const int mmwave_tx_pin;
extern const int poti_pin;
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
static const int MODE_FOCUS = 1;
static const int MODE_SHOWCASE = 2;
static const int MODE_CANVAS = 3;
extern bool always_on;

// Focus session (mode 1): arrival warmup -> deep focus
enum FocusPhase {
  FOCUS_NONE = 0,
  FOCUS_ARRIVAL,
  FOCUS_DEEP,
};

static const unsigned long FOCUS_WARMUP_MS = 30000;

extern FocusPhase focus_phase;

static const unsigned long TRANSITION_MS = 800;
static const unsigned long TRANSITION_PRESENCE_MS = 280;

// Focus candle (WLED Candle Multi port)
static const unsigned CANDLE_FRAME_MS = 25;
static const uint8_t CANDLE_SPEED = 96;
static const uint8_t CANDLE_INTENSITY = 224;
static const CRGB FOCUS_CANDLE_BRIGHT(255, 40, 0);
static const CRGB FOCUS_CANDLE_DIM(50, 6, 0);
static const uint8_t FOCUS_DEEP_HUE = 160;
static const uint8_t FOCUS_DEEP_SAT = 180;
static const uint8_t FOCUS_DEEP_VAL = 220;

struct VisualState {
  int mode;
  FocusPhase focus_phase;
  bool active;
};

// Modulation (computed by Logic layer, passed into Render)
struct LightMod {
  uint8_t color_mode;
  float energy;
  uint8_t brightness;
  float motion;
};

struct Poti {
  int pin;
  float smooth;
};

extern Poti poti;
extern LightMod mod;

// Raw poti sample from poll_inputs()
extern uint8_t g_poti_raw;

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
void update_focus_session();
void skip_focus_warmup();
void toggle_focus_color();
void compute_modulation();
void update_mode_button_pending();
void update_daynight_schedule();

void schedule_init();
bool schedule_time_valid();
int schedule_local_hour();
bool schedule_in_focus_hours(int hour);

void showcase_reset();
void showcase_update(int16_t enc1_delta, int16_t enc2_delta, unsigned long now_ms);
void update_showcase_inputs();

void canvas_apply_defaults(uint8_t default_hue, uint8_t default_sat);
void canvas_tuning_tick();
void canvas_tuning_update(int16_t enc1_delta, int16_t enc2_delta);
void render_canvas(unsigned long now_ms);
uint8_t canvas_hue_val();
void update_canvas_inputs();

// --- Render layer (no input reads, no state mutation) ---
void render_off();
void render_focus(const LightMod& mod, FocusPhase phase);
void render_showcase();
void render_visual_state_to(CRGB* buf, const VisualState& vs);
void focus_candle_reset();
void render_focus_candle(CRGB* out);
void render_focus_candle(CRGB* out, CRGB bright, CRGB dim);

void focus_tuning_update(int16_t enc1_delta, int16_t enc2_delta, FocusPhase phase);
void focus_tuning_tick();
void update_focus_inputs();
uint8_t focus_candle_speed_val();
uint8_t focus_candle_intensity_val();
uint8_t focus_deep_hue_val();
uint8_t focus_deep_sat_val();
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
uint8_t read_poti_impl(Poti& p);
