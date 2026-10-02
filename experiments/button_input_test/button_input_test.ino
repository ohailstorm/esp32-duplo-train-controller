#include <Arduino.h>

// These tests use hardware GPIO numbers, not remapped Arduino pin numbers.
#if defined(BOARD_HAS_PIN_REMAP) && !defined(BOARD_USES_HW_GPIO_NUMBERS)
#error "Remapped board selected. For Waveshare S3-Zero choose ESP32S3 Dev Module, not Ozobot DRVKit."
#endif

// GPIO numbers, not physical header positions. See this folder's README.
#if defined(ARDUINO_XIAO_ESP32C3)
static constexpr uint8_t buttonPins[] = {5, 6, 7, 10}; // D3, D4, D5, D10
#elif defined(ARDUINO_ESP32S3_DEV) && defined(CONFIG_IDF_TARGET_ESP32S3)
static constexpr uint8_t buttonPins[] = {4, 5, 6, 7}; // Waveshare S3-Zero
#else
#error "Select ESP32S3 Dev Module (S3-Zero) or XIAO_ESP32C3; check the wiring guide."
#endif

static constexpr uint32_t debounceMs = 25;
struct ButtonState {
  bool rawPressed;
  bool stablePressed;
  bool armed;
  uint32_t changedAt;
};
static ButtonState buttons[4];

void setup() {
  Serial.begin(115200);
  const uint32_t started = millis();
  while (!Serial && millis() - started < 5000) delay(10);
  Serial.println("\nFour-button input test. No BLE or train commands.");
  Serial.printf("Compiled board: %s; hardware GPIO numbering.\n", ARDUINO_BOARD);
  for (size_t i = 0; i < 4; ++i) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    // Require a stable release before accepting the first press, including boot.
    buttons[i] = {digitalRead(buttonPins[i]) == LOW, true, false, millis()};
    Serial.printf("B%u: GPIO%u; release to arm.\n", unsigned(i + 1), buttonPins[i]);
  }
}

void loop() {
  const uint32_t now = millis();
  for (size_t i = 0; i < 4; ++i) {
    auto& button = buttons[i];
    const bool pressed = digitalRead(buttonPins[i]) == LOW;
    if (pressed != button.rawPressed) {
      button.rawPressed = pressed;
      button.changedAt = now;
    }
    if (now - button.changedAt < debounceMs || pressed == button.stablePressed) continue;
    button.stablePressed = pressed;
    if (!button.armed) {
      if (!pressed) {
        button.armed = true;
        Serial.printf("B%u ready\n", unsigned(i + 1));
      }
      continue;
    }
    Serial.printf("B%u %s\n", unsigned(i + 1), pressed ? "pressed" : "released");
  }
  delay(1);
}
