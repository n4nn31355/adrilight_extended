#include <Arduino.h>

typedef void (*brightnessChangeCallback)(int8_t);

class BrightnessChanger
{
public:
  BrightnessChanger(
    unsigned long tickPeriodTime,
    brightnessChangeCallback callback,
    int8_t brightnessStep);

  void start();
  void stop();

  void tick();

private:
  unsigned long _previousMillis = 0;
  bool _active = false;

  // Milliseconds of change delay
  unsigned long _tickPeriodTime;

  brightnessChangeCallback _callback = NULL;
  int8_t _brightnessStep;
};
