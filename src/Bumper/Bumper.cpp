#include "./Bumper.h"

Bumper::Bumper(int id, int ledNum, int btnPin, int buzzerPin, Adafruit_TLC59711* ledDriver) {
  _id = id;
  _ledNum = ledNum;
  _btn = new Button(btnPin, INPUT_PULLUP);
  _buzzerPin = buzzerPin;
  pinMode(_buzzerPin, OUTPUT);
  _ledDriver = ledDriver;
}

void Bumper::rgb(uint16_t r, uint16_t g, uint16_t b) {
  _ledDriver->setLED(_ledNum, r, g, b);
  _ledDriver->write();
}

void Bumper::buzz(bool onOff) {
  digitalWrite(_buzzerPin, onOff);
}

bool Bumper::isPressed() {
  return _btn->uniquePress();
}

bool Bumper::isReleased() {
  return (_btn->stateChanged() && !_btn->isPressed());
}

int Bumper::getId() {
  return _id;
}
