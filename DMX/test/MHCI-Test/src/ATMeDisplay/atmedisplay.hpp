#pragma once

#define LOG_LOCAL_LEVEL LOG_LEVEL
#include "esp_log.h"

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1327.h>
#include <Wire.h>

#include <Timeout/timeout.hpp>

class ATMeDisplay {
public:
    ATMeDisplay() : 
        display(128, 128, &Wire, -1, 1000000), 
        flashingDisplay(500),
        displayUpdate(250),
        displayInvert(false),
        TAG("ATMeDisplay") {};

    ~ATMeDisplay() {};

    bool begin(unsigned long now);
    bool update(unsigned long now, bool forceUpdate, uint8_t fan, float haze, std::string generatorState, uint8_t hazeValue, bool unitOn, bool hazeOn);

private:
    const char* TAG;
    Adafruit_SSD1327 display;

    Timeout flashingDisplay;
    Timeout displayUpdate;
    bool displayInvert;

    void centreText(Adafruit_SSD1327 &display, const std::string text, int y);

};
