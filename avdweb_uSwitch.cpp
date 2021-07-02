/*
 * avdweb_uSwitch.cpp
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

HISTORY:
1.0.0    20-04-2013 _debouncePeriod=50
1.0.1    22-05-2013 Added longPress, doubleClick
1.0.2    01-12-2015 added process(input)
1.0.3    15-01-2016 added deglitching
1.0.5    25-01-2017 file renamed to avdweb_uSwitch
1.1.0    28-07-2018 added callbacks (code by Sean Lanigan, added by Martin Laclaustra)
1.2.0-rc 28-07-2018 added singleclick. Reorganize, keeping variables for each event in one function
1.2.0    29-09-2018 released
1.2.1    30-11-2018 bugfix. Initialize time variables in the constructor. Fixes false event if first call to poll was delayed
1.2.2    18-10-2019 beep when a switch is pressed with using a setBeepAllCallback function
1.2.3    03-04-2020 made public: deglitchPeriod, debouncePeriod, longPressPeriod, doubleClickPeriod

..........................................DEGLITCHING..............................

                        ________________   _
               on      |                | | |    _
                       |                | | |   | |
                       |                |_| |___| |__
 analog        off_____|_____________________________|____________________________

                        ________________   _     _
 input            _____|                |_| |___| |_______________________________

 poll            ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^

 equal           0 1 1 0 1 1 1 1 1 1 1 1 0 0 0 1 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1

 SWITCH_DEGLITCH_PERIOD  <--------><--   <--     <-  <--------><--------><--------
                                    ___________________________
 deglitched       _________________|                           |__________________

 deglitchTime            ^         ^     ^       ^   ^         ^        ^

 ..........................................DEBOUNCING.............................

 SWITCH_DEBOUNCE_PERIOD            <-------------------------------->
                                    _________________________________
 debounced        _________________|                                 |____________
                                    _                                 _
 _switched        _________________| |_______________________________| |__________

 switchedTime                      ^                                 ^


**********************************************************************************
........................................DOUBLE CLICK..............................

                           __________         ______
 debounced        ________|          |_______|      |_____________________________

 poll            ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^
                           _                  _
 pushed          _________| |________________| |__________________________________

 pushedTime               ^                  ^
                                      _              _
 released        ____________________| |____________| |___________________________

 releasedTime                        ^              ^

 SWITCH_DOUBLE_CLICK_PERIOD<------------------------------------->
                                              _
 _doubleClick     ___________________________| |__________________________________


........................................LONG PRESS................................

                           ___________________________
 debounced        ________|                           |___________________________

 poll            ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^

 SWITCH_LONG_PRESS_PERIOD   <--------------->
                            _                           _
 _switched        _________| |_________________________| |________________________
                                              __________
 longPressDisable ___________________________|          |_________________________
                                              _
 _longPress       ___________________________| |__________________________________


........................................SINGLE CLICK..............................

                           __________                                 ______
 debounced        ________|          |_______________________________|      |_____

 poll            ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^

 SWITCH_LONG_PRESS_PERIOD  <--------------->
 SWITCH_DOUBLE_CLICK_PERIOD<------------------------------------->
                            _         _                               _      _
 _switched        _________| |_______| |_____________________________| |____| |___
                                                                  _____
 singleClickDisable______________________________________________|     |__________
                                                                  _
 _singleClick     _______________________________________________| |______________

*/
// clang-format on

#include <Arduino.h>
#include "avdweb_uSwitch.h"

Switch::Switch(byte _pin, byte PinMode, bool polarity) : pin(_pin), polarity(polarity) {
  pinMode(pin, PinMode);
  switchedTime = millis();
  debounced = digitalRead(pin);
  singleClickDisable = true;
  poll();
}

bool Switch::poll() {
  input = digitalRead(pin);
  uint32_t ms = millis();
  return process(ms);
}

bool Switch::process(uint32_t ms) {
  deglitch(ms);
  debounce(ms);
  calcSingleClick(ms);
  calcDoubleClick(ms);
  calcLongPress(ms);
  if (switched()) {
    switchedTime = ms; // stores last times for future rounds
    if (pushed()) {
      if (_callback)
        _callback(BEEP_ALL);
      pushedTime = ms;
    } else {
      releasedTime = ms;
    }
  }
  triggerCallbacks();
  return _switched;
}

void inline Switch::deglitch(uint32_t ms) {
  if (input == lastInput)
    equal = 1;
  else {
    equal = 0;
    deglitchTime = ms;
  }
  if (equal && ((ms - deglitchTime) > SWITCH_DEGLITCH_PERIOD)) // max 50ms, disable deglitch: 0ms
  {
    deglitched = input;
    deglitchTime = ms;
  }
  lastInput = input;
}

void inline Switch::debounce(uint32_t ms) {
  _switched = 0;
  if ((deglitched != debounced) && ((ms - switchedTime) > SWITCH_DEBOUNCE_PERIOD)) {
    debounced = deglitched;
    _switched = 1;
  }
}

void inline Switch::calcSingleClick(uint32_t ms) {
  _singleClick = false;
  if (pushed()) {
    if ((ms - pushedTime) >= SWITCH_DOUBLE_CLICK_PERIOD) {
      singleClickDisable = false; // resets when pushed not in second click of doubleclick
    } else {
      singleClickDisable = true; // silence single click in second cl. doublecl.
    }
  }
  if (!singleClickDisable) {
    _singleClick =
        !switched() && !on() && ((releasedTime - pushedTime) <= SWITCH_LONG_PRESS_PERIOD) &&
        ((ms - pushedTime) >= SWITCH_DOUBLE_CLICK_PERIOD); // true just one time between polls
    singleClickDisable = _singleClick;                     // will be reset at next push
  }
}

void inline Switch::calcDoubleClick(uint32_t ms) {
  _doubleClick = pushed() && ((ms - pushedTime) < SWITCH_DOUBLE_CLICK_PERIOD);
}

void inline Switch::calcLongPress(uint32_t ms) {
  _longPress = false;
  if (released())
    longPressDisable = false; // resets when released
  if (!longPressDisable) {
    _longPress = !switched() && on() &&
                 ((ms - pushedTime) > SWITCH_LONG_PRESS_PERIOD); // true just one time between polls
    longPressDisable = _longPress;                               // will be reset at next release
  }
}

bool Switch::switched() {
  return _switched;
}

bool Switch::on() {
  return !(debounced ^ polarity);
}

bool Switch::pushed() {
  return _switched && !(debounced ^ polarity);
}

bool Switch::released() {
  return _switched && (debounced ^ polarity);
}

bool Switch::longPress() {
  return _longPress;
}

bool Switch::doubleClick() {
  return _doubleClick;
}

bool Switch::singleClick() {
  return _singleClick;
}

void Switch::triggerCallbacks() {
  if (_callback) {
    if (pushed()) {
      _callback(PUSHED);
    } else if (released()) {
      _callback(RELEASED);
    }

    if (longPress()) {
      _callback(LONG_PRESS);
    }

    if (doubleClick()) {
      _callback(DOUBLE_CLICK);
    }

    if (singleClick()) {
      _callback(SINGLE_CLICK);
    }
  }
}

void Switch::setCallback(switchCallbackFunc_t cb) {
  _callback = cb; // Store the callback function
}
