#include "../hardware.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Encoder module implementation in inputs/.

struct EncEvent {
  uint8_t enc;     // 1 or 2
  uint8_t state;   // (A<<1)|B
  unsigned long ts; // micros()
};

static QueueHandle_t _enc_queue = NULL;
static const int ENC_QUEUE_LEN = 32;

class EncoderDecoder {
public:
  EncoderDecoder() : _pos(0), _last_state(0), _last_ts(0) {}
  void begin(uint8_t init_state) { _last_state = init_state; _last_ts = micros(); }
  void process_event(const EncEvent &e) {
    unsigned long now = e.ts;
    if ((now - _last_ts) < 200) return; // 0.2ms guard
    _last_ts = now;
    uint8_t curr = e.state & 0x3;
    if (curr == _last_state) return;
    int8_t delta = _lookup[(_last_state << 2) | curr];
    _pos += delta;
    _last_state = curr;
  }
  long get_pos() {
    long v;
    noInterrupts();
    v = _pos;
    interrupts();
    return v;
  }
private:
  long _pos;
  uint8_t _last_state;
  unsigned long _last_ts;
  static const int8_t _lookup[16];
};

const int8_t EncoderDecoder::_lookup[16] = {
  0, -1,  1,  0,
  1,  0,  0, -1,
 -1,  0,  0,  1,
  0,  1, -1,  0
};

static EncoderDecoder _enc1;
static EncoderDecoder _enc2;

// ISR wrappers
void IRAM_ATTR _enc1_isr() {
  if (!_enc_queue) return;
  EncEvent ev;
  ev.enc = 1;
  ev.state = (digitalRead(encoder1_a_pin) << 1) | digitalRead(encoder1_b_pin);
  ev.ts = micros();
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(_enc_queue, &ev, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

void IRAM_ATTR _enc2_isr() {
  if (!_enc_queue) return;
  EncEvent ev;
  ev.enc = 2;
  ev.state = (digitalRead(encoder2_a_pin) << 1) | digitalRead(encoder2_b_pin);
  ev.ts = micros();
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(_enc_queue, &ev, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

static void enc_worker_task(void *pv) {
  EncEvent ev;
  for (;;) {
    if (xQueueReceive(_enc_queue, &ev, portMAX_DELAY) == pdTRUE) {
      if (ev.enc == 1) _enc1.process_event(ev);
      else _enc2.process_event(ev);
    }
  }
}

// implementation-suffixed functions are already present in this file.
void init_encoders_impl() {
  pinMode(encoder1_a_pin, INPUT_PULLUP);
  pinMode(encoder1_b_pin, INPUT_PULLUP);
  pinMode(encoder2_a_pin, INPUT_PULLUP);
  pinMode(encoder2_b_pin, INPUT_PULLUP);

  if (!_enc_queue) {
    _enc_queue = xQueueCreate(ENC_QUEUE_LEN, sizeof(EncEvent));
    if (_enc_queue) {
      xTaskCreatePinnedToCore(enc_worker_task, "enc_worker", 4096, NULL, 2, NULL, 1);
    }
  }

  _enc1.begin((digitalRead(encoder1_a_pin) << 1) | digitalRead(encoder1_b_pin));
  _enc2.begin((digitalRead(encoder2_a_pin) << 1) | digitalRead(encoder2_b_pin));

  attachInterrupt(digitalPinToInterrupt(encoder1_a_pin), _enc1_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoder1_b_pin), _enc1_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoder2_a_pin), _enc2_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoder2_b_pin), _enc2_isr, CHANGE);
}

long get_encoder1_pos_impl() { return _enc1.get_pos(); }
long get_encoder2_pos_impl() { return _enc2.get_pos(); }

void poll_encoders_impl() {
  EncEvent ev;
  ev.ts = micros();

  ev.enc = 1;
  ev.state = (digitalRead(encoder1_a_pin) << 1) | digitalRead(encoder1_b_pin);
  _enc1.process_event(ev);

  ev.enc = 2;
  ev.state = (digitalRead(encoder2_a_pin) << 1) | digitalRead(encoder2_b_pin);
  _enc2.process_event(ev);
}
