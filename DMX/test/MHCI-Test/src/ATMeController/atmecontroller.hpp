#pragma once

#define LOG_LOCAL_LEVEL LOG_LEVEL
#include "esp_log.h"

#include "DMXFixture/dmxfixture.hpp"
#include "ATMeDisplay/atmedisplay.hpp"
#include "ATMeDMX/atmedmx.hpp"
#include "ATMeEncoder/atmeencoder.hpp"

//#include <Encoder/encoder.hpp>


class ATMeController {
public:
    ATMeController();

    bool begin(unsigned long now);
    bool update(unsigned long now);
private:
    const char* TAG;

    ATMeDisplay display;
    ATMeDMX dmx;

    bool triggerDisplayUpdate = false;

    ATMeEncoder fanEncoder;
    ATMeEncoder hazeEncoder;

    // Encoder fan;
    // Encoder haze;


    bool unitOn = true;
    bool hazeOn = false;
};
