#pragma once

#define LOG_LOCAL_LEVEL LOG_LEVEL
#include "esp_log.h"

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1327.h>
#include <Wire.h>

#include <Timeout/timeout.hpp>
#include <ATMeController/atmecontroller.hpp>

class ATMeController;


class ATMeDisplay {
public:
    ATMeDisplay() : 
        display(128, 128, &Wire, -1, 1000000), 
        flashingDisplayTimeout(500),
        displayUpdate(250),
        flashingBackgroundColour(true),
        TAG("ATMeDisplay") {};

    ~ATMeDisplay() {};

    bool begin(unsigned long now);
    bool update(unsigned long now, bool forceUpdate, ATMeController& atmeController, bool alert);

private:
    const char* TAG;
    Adafruit_SSD1327 display;

    Timeout flashingDisplayTimeout;
    Timeout displayUpdate;
    bool flashingBackgroundColour; // White = 1, black = 0

    void drawStateGraphics(Adafruit_SSD1327 &display, ATMeController& atmeController, uint16_t backgroundColour, uint16_t textColour);
    void drawCentredText(Adafruit_SSD1327 &display, const std::string text, int y);
    void percentageValue(Adafruit_SSD1327 &display, const uint16_t percentage, int x, int y);
    void dmxValue(Adafruit_SSD1327 &display, const uint16_t percentage, int x, int y);

};
