#include <Arduino.h>

#include <FastLED.h>
#include <OneButton.h>

#include "Buttons.h"
#include "display.h"

#define NUM_LEDS (2 * 35 + 2 * 19)
#define CURRENT_LIMIT_MILLIAMPS 750
#define LED_DATA_PIN 6

// range is 0..255 with 255 beeing the MAX brightness
#define BRIGHTNESS_DEFAULT 120
#define BRIGHTNESS_MIN 30
#define BRIGHTNESS_IDLE 30

#define BRIGHTNESS_CHANGE_STEP 15

#define MINUS_BTN_PIN 16
#define PLUS_BTN_PIN 14

// --------------------------------------------------------------------------------------------
// NO CHANGE REQUIRED BELOW THIS LINE
// --------------------------------------------------------------------------------------------

#define UPDATES_PER_SECOND 60
#define TIMEOUT 3000
#define MODE_ANIMATION 0
#define MODE_AMBILIGHT 1
#define MODE_BLACK 2
uint8_t mode = MODE_ANIMATION;

bool isBrightnessMinEditEnabled = false;
uint8_t brightnessCurrent = BRIGHTNESS_DEFAULT;
uint8_t brightnessMax = BRIGHTNESS_DEFAULT;
uint8_t brightnessMin = BRIGHTNESS_MIN;
uint8_t brightnessIdle = BRIGHTNESS_IDLE;

byte MESSAGE_PREAMBLE[] = { 0x00, 0x01, 0x02, 0x03, 0x04,
                            0x05, 0x06, 0x07, 0x08, 0x09 };
uint8_t PREAMBLE_LENGTH = 10;
uint8_t current_preamble_position = 0;

unsigned long last_serial_available = -1L;

CRGB leds[NUM_LEDS];
CRGB ledsTemp[NUM_LEDS];
byte buffer[3];

// Filler animation attributes
CRGBPalette16 currentPalette = RainbowColors_p;
TBlendType currentBlending = LINEARBLEND;
uint8_t startIndex = 0;

OneButton minusBtn = OneButton(MINUS_BTN_PIN, true, true);
OneButton plusBtn = OneButton(PLUS_BTN_PIN, true, true);

BrightnessChanger* brightnessIncreaser = NULL;
BrightnessChanger* brightnessDecreaser = NULL;

void
drawDisplay()
{
  switch (mode) {
    case MODE_ANIMATION:
      display::draw(
        brightnessIdle, brightnessMin, isBrightnessMinEditEnabled, "ANIM");
      break;

    case MODE_AMBILIGHT:
      display::draw(
        brightnessMax, brightnessMin, isBrightnessMinEditEnabled, "AMBI");
      break;

    case MODE_BLACK:
      display::draw(0, brightnessMin, isBrightnessMinEditEnabled, "BLACK");
  }
}

void
changeBrightness(int8_t value)
{
  uint8_t* brightness;

  if (isBrightnessMinEditEnabled) {
    brightness = &brightnessMin;
  } else {
    switch (mode) {
      case MODE_ANIMATION:
        brightness = &brightnessIdle;
        break;

      case MODE_AMBILIGHT:
        brightness = &brightnessMax;
        break;

      case MODE_BLACK:
        return;
    }
  }

  *brightness += value;

  drawDisplay();
}

void
fillLEDsFromPaletteColors()
{
  startIndex++;

  uint8_t colorIndex = startIndex;
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(
      currentPalette, colorIndex, brightnessIdle, currentBlending);
    colorIndex += 3;
  }

  FastLED.delay(1000 / UPDATES_PER_SECOND);

  if (Serial.available() > 0) {
    mode = MODE_AMBILIGHT;
    drawDisplay();
  }
}

void
showBlack()
{
  if (brightnessCurrent > 0) {
    brightnessCurrent--;
    FastLED.setBrightness(brightnessCurrent);
  }

  FastLED.delay(1000 / UPDATES_PER_SECOND);

  if (Serial.available() > 0) {
    mode = MODE_AMBILIGHT;
    drawDisplay();
  }
}

bool
waitForPreamble(int timeout)
{
  last_serial_available = millis();
  current_preamble_position = 0;
  while (current_preamble_position < PREAMBLE_LENGTH) {
    if (Serial.available() > 0) {
      last_serial_available = millis();

      if (Serial.read() == MESSAGE_PREAMBLE[current_preamble_position]) {
        current_preamble_position++;
      } else {
        current_preamble_position = 0;
      }
    }

    if (millis() - last_serial_available > timeout) {
      return false;
    }
  }
  return true;
}

void
processIncomingData()
{
  if (waitForPreamble(TIMEOUT)) {
    for (int ledNum = 0; ledNum < NUM_LEDS + 1; ledNum++) {
      // we always have to read 3 bytes (RGB!)
      // if it is less, we ignore this frame and wait for the next preamble
      if (Serial.readBytes((char*)buffer, 3) < 3)
        return;

      if (ledNum < NUM_LEDS) {
        byte blue = brightnessMin > buffer[0] ? brightnessMin : buffer[0];
        byte green = brightnessMin > buffer[1] ? brightnessMin : buffer[1];
        byte red = brightnessMin > buffer[2] ? brightnessMin : buffer[2];
        ledsTemp[ledNum] = CRGB(red, green, blue);
      } else if (ledNum == NUM_LEDS) {
        // this must be the "postamble"
        // this last "color" is actually a closing preamble
        // if the postamble does not match the expected values, the colors will
        // not be shown
        if (buffer[0] == 85 && buffer[1] == 204 && buffer[2] == 165) {
          // the preamble is correct, update the leds!

          // TODO: can we flip the used buffer instead of copying the data?
          for (int ledNum = 0; ledNum < NUM_LEDS; ledNum++) {
            leds[ledNum] = ledsTemp[ledNum];
          }

          if (brightnessCurrent < brightnessMax) {
            brightnessCurrent++;
            FastLED.setBrightness(brightnessCurrent);
          } else if (brightnessCurrent > brightnessMax) {
            brightnessCurrent--;
            FastLED.setBrightness(brightnessCurrent);
          }

          // send LED data to actual LEDs
          FastLED.show();
        }
      }
    }
  } else {
    // if we get here, there must have been data before(so the user already
    // knows, it works!) simply go to black!
    mode = MODE_BLACK;
    drawDisplay();
  }
}

void
setup()
{
  Serial.begin(1000000);

  // Actually minimal tick period limited by display refresh rate
  BrightnessChanger a = BrightnessChanger(500, changeBrightness, 5);
  BrightnessChanger b = BrightnessChanger(500, changeBrightness, -5);

  brightnessIncreaser = &a;
  brightnessDecreaser = &b;

  display::init();
  drawDisplay();

  FastLED.clear(true);
  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, NUM_LEDS);

  FastLED.setMaxPowerInVoltsAndMilliamps(5, CURRENT_LIMIT_MILLIAMPS);
  FastLED.setBrightness(brightnessCurrent);
  FastLED.setDither(0);

  minusBtn.attachDoubleClick([]() {
    if (mode == MODE_BLACK) {
      mode = MODE_ANIMATION;
      FastLED.setBrightness(brightnessIdle);
    } else {
      mode = MODE_BLACK;
    }
    drawDisplay();
  });

  plusBtn.attachDoubleClick([]() {
    isBrightnessMinEditEnabled = !isBrightnessMinEditEnabled;
    drawDisplay();
  });

  minusBtn.attachClick([]() { changeBrightness(-BRIGHTNESS_CHANGE_STEP); });
  plusBtn.attachClick([]() { changeBrightness(BRIGHTNESS_CHANGE_STEP); });

  minusBtn.attachLongPressStart([]() { brightnessDecreaser->start(); });
  minusBtn.attachLongPressStop([]() { brightnessDecreaser->stop(); });

  plusBtn.attachLongPressStart([]() { brightnessIncreaser->start(); });
  plusBtn.attachLongPressStop([]() { brightnessIncreaser->stop(); });
}

void
loop()
{
  minusBtn.tick();
  plusBtn.tick();

  brightnessIncreaser->tick();
  brightnessDecreaser->tick();

  switch (mode) {
    case MODE_ANIMATION:
      fillLEDsFromPaletteColors();
      break;

    case MODE_AMBILIGHT:
      processIncomingData();
      break;

    case MODE_BLACK:
      showBlack();
      break;
  }
}
