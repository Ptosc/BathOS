#include "hardware.h"

// ===== REAL DEFINITIONS =====

CRGB leds[NUMPIXELS];
CRGB status_led[STATUS_LED_COUNT];
CRGB prev_frame[NUMPIXELS];
CRGB new_frame[NUMPIXELS];
CRGB last_unscaled[NUMPIXELS];

int raw_distance = -1;
int filtered_distance = -1;

float presence = 0.0;
float sensor_presence = 0.0; // from active sensor (mmWave)

int mode = 0;
const int max_modes = 4; // off, focus, showcase, canvas
bool always_on = false;

FocusPhase focus_phase = FOCUS_NONE;

LightMod mod;

// Define potentiometers in a single place (global definitions)
Poti poti  = {32, 0};