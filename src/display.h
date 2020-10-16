#include <Arduino.h>

namespace display {
void
init();
void
draw(
  const uint8_t brightness,
  const uint8_t brightnessMin,
  const bool isBrightnessMinEditEnabled,
  const char modeName[]);
} // namespace display
