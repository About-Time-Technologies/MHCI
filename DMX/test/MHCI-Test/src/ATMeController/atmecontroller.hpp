#pragma once

#define LOG_LOCAL_LEVEL LOG_LEVEL
#include "esp_log.h"

#include <string>

#include "DMXFixture/dmxfixture.hpp"
#include "ATMeDisplay/atmedisplay.hpp"
#include "ATMeDMX/atmedmx.hpp"
#include "ATMeEncoder/atmeencoder.hpp"
#include <stdint.h>
#include <Preferences.h>

class ATMeDisplay;
class ATMeDMX;

enum ATMeInputState {
    INPUT_CONTROL,
    INPUT_LEDs,
    INPUT_ADDRESSES,
    INPUT_PURGE_HOLD,
    INPUT_PURGE_ACTIVE,
    COUNT
};

class ATMeController {
public:
    ATMeController(ATMeDisplay* _display, ATMeDMX* _dmx);

    bool begin(unsigned long now);
    bool update(unsigned long now);

    uint16_t fanAddress;
    uint8_t fanValue;

    uint16_t hazeAddress;
    bool unitOn;
    uint8_t hazeLevel;
    bool hazeOn;

    uint8_t frontLEDLevel;
    uint8_t rearLEDLevel;

    std::string getATMeStateString();
    std::string getInputStateString();

    ATMeInputState getInputState();

    const uint PWM_CHANNEL = 0;
    const uint PWM_FREQ = 500;
    const uint PWM_RESOLUTION = 8;
    

private:
    const char* TAG;
    void nextState();
    void saveAddresses();
    void loadAddresses();

    int32_t constrainAddition(int32_t input, int32_t addition, int32_t min, int32_t max, int32_t nearest);
    void processControlInputs(int32_t fanDelta, int32_t hazeDelta, bool fanButton, bool hazeButton);
    void processAddressInputs(int32_t fanDelta, int32_t hazeDelta, bool fanButton, bool hazeButton);
    void processLEDInputs(int32_t fanDelta, int32_t hazeDelta, bool fanButton, bool hazeButton);

    ATMeDisplay* display;
    ATMeDMX* dmx;

    bool triggerDisplayUpdate = false;

    ATMeEncoder fanEncoder;
    ATMeEncoder hazeEncoder;
    ATMeInputState inputState;

    bool hazeLongPress;
    bool fanLongPress;

    Preferences preferences;

    Timeout menuTimeout;
};
