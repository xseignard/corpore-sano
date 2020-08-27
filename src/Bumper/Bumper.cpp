#include "./Bumper.h"

#define RGB_DELAY 5

Bumper::Bumper(int id, int ledNum, int btnPin, int buzzerPin, Adafruit_TLC59711* ledDriver) {
  _id = id;
  _ledNum = ledNum;
  _btn = new Button(btnPin, INPUT_PULLUP);
  _buzzerPin = buzzerPin;
  pinMode(_buzzerPin, OUTPUT);
  _ledDriver = ledDriver;
  _debounce = 50;
  _lastPress = millis();
  _lastRelease = millis();
  _lastRGB = millis();
}

void Bumper::rgb(uint16_t r, uint16_t g, uint16_t b) {
  _lastRGB = millis();
  _ledDriver->setLED(_ledNum, r, g, b);
  _ledDriver->write();
}

void Bumper::buzz(bool onOff) {
  digitalWrite(_buzzerPin, onOff);
}

bool Bumper::isPressed() {
  unsigned long now = millis();
  if (now - _lastPress > _debounce && now - _lastRGB > RGB_DELAY) {
    _lastPress = now;
    return _btn->uniquePress();
  }
  else return false;
}

bool Bumper::isReleased() {
  unsigned long now = millis();
  if (now - _lastRelease > _debounce && now - _lastRGB > RGB_DELAY) {
    _lastRelease = now;
    return (_btn->stateChanged() && !_btn->isPressed());
  }
  else return false;
}

void Bumper::setDebounce(int debounce) {
  _debounce = debounce;
}

int Bumper::getId() {
  return _id;
}
