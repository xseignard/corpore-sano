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
  int getId();

 private:
  int _id;
  int _ledNum;
  Button* _btn;
  int _buzzerPin;
  Adafruit_TLC59711* _ledDriver;
};

#endif
