#include "./Bumper.h"

#define RGB_DELAY 50

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
  _prevPress = false;
  _prevRelease = false;
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
  if (now - _lastPress > _debounce) {
    _lastPress = now;
    int j = 0;
    for (int i = 0; i < 20; i++) {
      bool current = _btn->isPressed();
      if (current) j++;
      delayMicroseconds(100);
    }
    bool result = j > 15 && !_prevPress;
    _prevPress = j > 15;
    return result;
  }
  else return false;
}

bool Bumper::isReleased() {
  unsigned long now = millis();
  if (now - _lastRelease > _debounce) {
    _lastRelease = now;
    int j = 0;
    for (int i = 0; i < 20; i++) {
      bool current = !_btn->isPressed();
      if (current) j++;
      delayMicroseconds(100);
    }
    bool result = j > 15 && !_prevRelease;
    _prevRelease = j > 15;
    return result;
  }
  else return false;
}

void Bumper::setDebounce(int debounce) {
  _debounce = debounce;
}

int Bumper::getId() {
  return _id;
}
