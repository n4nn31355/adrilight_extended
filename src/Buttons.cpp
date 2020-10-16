#include "Buttons.h"

BrightnessChanger::BrightnessChanger(
  unsigned long tickPeriodTime,
  brightnessChangeCallback callback,
  int8_t brightnessStep)
{
  _tickPeriodTime = tickPeriodTime;
  _callback = callback;
  _brightnessStep = brightnessStep;
};

void
BrightnessChanger::start()
{
  _active = true;
}

void
BrightnessChanger::stop()
{
  _active = false;
}

void
BrightnessChanger::tick()
{
  unsigned long currentMillis = millis();

  if (
    (_active == true) && (currentMillis - _previousMillis >= _tickPeriodTime)) {
    _previousMillis = currentMillis;
    _callback(_brightnessStep);
  }
}
