#include <U8g2lib.h>
#include <Wire.h>

#include "display.h"

namespace display {
U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, SCL, SDA);

void
init()
{
  u8g2.begin();
  u8g2.setFont(u8g2_font_tenfatguys_tu);
  // TODO: Disable display when idle
  // u8g2.setPowerSave(0);
}

void
draw(
  const uint8_t brightness,
  const uint8_t brightnessMin,
  const bool isBrightnessMinEditEnabled,
  const char modeName[])
{
  u8g2.firstPage();
  do {
    uint8_t brEditOffset = 0;
    if (isBrightnessMinEditEnabled) {
      brEditOffset = 11;
    }
    u8g2.drawTriangle(
      118, 1 + brEditOffset, 118, 9 + brEditOffset, 110, 5 + brEditOffset);

    u8g2_uint_t y = 10;

    u8g2.setCursor(0, y);
    u8g2.print(F("BR: "));

    u8g2.setCursor(72, y);
    u8g2.print(u8x8_u8toa(brightness, 3));
    u8g2.setDrawColor(1);

    y = 10 + 11;
    u8g2.setCursor(0, y);
    u8g2.print(F("BR MIN: "));

    u8g2.setCursor(72, y);
    u8g2.print(u8x8_u8toa(brightnessMin, 3));
    u8g2.setDrawColor(1);

    y = 10 + 11 + 11;
    u8g2.setCursor(0, y);
    u8g2.print(F("MODE: "));

    u8g2.setCursor(72, y);
    u8g2.print(modeName);
  } while (u8g2.nextPage());
}
} // namespace display
