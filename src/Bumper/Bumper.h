#ifndef Bumper_H
#define Bumper_H

#include <Adafruit_TLC59711.h>
#include <Arduino.h>
#include <Button.h>

class Bumper {
 public:
  Bumper(int id, int ledNum, int btnPin, int buzzerPin, Adafruit_TLC59711* ledDriver);
  void rgb(uint16_t r, uint16_t g, uint16_t b);
  void buzz(bool onOff);
  bool isPressed();
  bool isReleased();
  void setDebounce(int debounce);
  int getId();

 private:
  int _id;
  int _ledNum;
  Button* _btn;
  int _buzzerPin;
  Adafruit_TLC59711* _ledDriver;
  int _debounce;
  unsigned long _lastPress;
  unsigned long _lastRelease;
  unsigned long _lastRGB;
  bool _prevPress;
  bool _prevRelease;
};

#endif
