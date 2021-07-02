/*
 * avdweb_uSwitch.h
 * Copyright (C) 2012  Albert van Dalen http://www.avdweb.nl (Original code)
 * Copyright (C) 2021  Marcel Toele ("Diet" modifications)
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version. This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License at http://www.gnu.org/licenses .
 */

// Original file header:
// clang-format off
/*
Switch.cpp
Copyright (C) 2012  Albert van Dalen http://www.avdweb.nl
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License at http://www.gnu.org/licenses .

AUTHOR: Albert van Dalen
WEBSITE: http://www.avdweb.nl/arduino/hardware-interfacing/simple-switch-debouncer.html
*/
// clang-format on

#pragma once

#include <Arduino.h>

typedef enum {
  PUSHED = 1,
  RELEASED,
  LONG_PRESS,
  DOUBLE_CLICK,
  SINGLE_CLICK,
  BEEP_ALL, // What is this supposed to do?
} switchCallback_t;

typedef void (*switchCallbackFunc_t)(switchCallback_t type);

#ifndef SWITCH_DEBOUNCE_PERIOD
#define SWITCH_DEBOUNCE_PERIOD 50
#endif

#ifndef SWITCH_LONG_PRESS_PERIOD
#define SWITCH_LONG_PRESS_PERIOD 300
#endif

#ifndef SWITCH_DOUBLE_CLICK_PERIOD
#define SWITCH_DOUBLE_CLICK_PERIOD 250
#endif

#ifndef SWITCH_DEGLITCH_PERIOD
#define SWITCH_DEGLITCH_PERIOD 10
#endif

class Switch {
public:
  Switch(byte _pin, byte PinMode = INPUT_PULLUP, bool polarity = LOW);
  bool poll();     // Returns 1 if switched
  bool switched(); // will be refreshed by poll()
  bool on();
  bool pushed();      // will be refreshed by poll()
  bool released();    // will be refreshed by poll()
  bool longPress();   // will be refreshed by poll()
  bool doubleClick(); // will be refreshed by poll()
  bool singleClick(); // will be refreshed by poll()

  // Set methods for event callbacks
  void setCallback(switchCallbackFunc_t cb);

protected:
  bool process(); // not inline, used in child class
  void inline deglitch();
  void inline debounce();
  void inline calcLongPress();
  void inline calcDoubleClick();
  void inline calcSingleClick();
  void triggerCallbacks();

  unsigned long deglitchTime, switchedTime, pushedTime, releasedTime, ms;
  const byte pin;
  const bool polarity : 1;
  bool input : 1, lastInput : 1, equal : 1, deglitched : 1, debounced : 1, _switched : 1,
      _longPress : 1, longPressDisable : 1, _doubleClick : 1, _singleClick : 1, singleClickDisable : 1;

  // Event callbacks
  switchCallbackFunc_t _callback = nullptr;
};
