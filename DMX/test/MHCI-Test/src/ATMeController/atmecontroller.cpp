#include "ATMeController/atmecontroller.hpp"
#include "ATMeDisplay/atmedisplay.hpp"
#include "ATMeDMX/atmedmx.hpp"
#include "ATMeEncoder/atmeencoder.hpp"

ATMeController::ATMeController(ATMeDisplay* _display, ATMeDMX* _dmx) :
    TAG("ATMeController"),
    inputState(ATMeInputState::INPUT_CONTROL),
    fanAddress(1),
    fanValue(0),
    hazeAddress(2),
    unitOn(true),
    hazeLevel(0),
    hazeOn(true),
    dmx(_dmx),
    display(_display)
    {};

bool ATMeController::begin(unsigned long now) {
    dmx->begin(now);
    display->begin(now);

    fanEncoder.begin(now, 0);
    hazeEncoder.begin(now, 1);

    fanEncoder.onButtonEvent(ButtonEvent::BUTTON_PRESS, []() { ESP_LOGD("Fan", "Press"); } );
    fanEncoder.onButtonEvent(ButtonEvent::BUTTON_DOUBLE_PRESS, []() { ESP_LOGD("Fan", "Double press"); } );
    fanEncoder.onButtonEvent(ButtonEvent::BUTTON_LONG_PRESS_START, []() { ESP_LOGD("Fan", "Long press"); } );

    hazeEncoder.onButtonEvent(ButtonEvent::BUTTON_PRESS, []() { ESP_LOGD("Haze", "Press"); } );
    hazeEncoder.onButtonEvent(ButtonEvent::BUTTON_DOUBLE_PRESS, []() { ESP_LOGD("Haze", "Double press"); } );
    hazeEncoder.onButtonEvent(ButtonEvent::BUTTON_LONG_PRESS_START, []() { ESP_LOGD("Haze", "Long press"); } );

    loadAddresses();

    return true;
}

bool ATMeController::update(unsigned long now) {
    int32_t fanDelta = fanEncoder.update(now);
    int32_t hazeDelta = hazeEncoder.update(now);

    if (fanDelta) ESP_LOGD(TAG, "Fan delta: %d", fanDelta);
    if (hazeDelta) ESP_LOGD(TAG, "Haze delta: %d", hazeDelta);

    if (fanDelta || hazeDelta) {

        switch (inputState) {
            case ATMeInputState::INPUT_CONTROL:
                processControlInputs(fanDelta, hazeDelta, fanEncoder.getButtonState(), hazeEncoder.getButtonState());
                break;
        }

    }

    dmx->update(now, *this);

    if (fanDelta || hazeDelta) triggerDisplayUpdate = true;



    if (triggerDisplayUpdate) {
        triggerDisplayUpdate = false; // Reset the flag after updating the display
        display->update(now, true, *this);

        fanEncoder.updateNeopixel(fanValue,0,0);
        hazeEncoder.updateNeopixel(0,hazeLevel,0);
    }

    display->update(now, false, *this);

    return true;
}

std::string ATMeController::getATMeStateString() {
    return dmx->getHazerStateString();
}

std::string ATMeController::getInputStateString() {
    switch(inputState) {
        case ATMeInputState::INPUT_CONTROL:
            return "CONTROL";
        case ATMeInputState::INPUT_ADDRESSES:
            return "ADDRESS";
        case ATMeInputState::INPUT_LEDs:
            return "LED";
        case ATMeInputState::INPUT_PURGE_HOLD:
            return "PURGE WAIT";
        case ATMeInputState::INPUT_PURGE_ACTIVE:
            return "PURGING";
    }
    return "ERR";
}

void ATMeController::nextState() {
    uint8_t current = static_cast<uint8_t>(inputState);
    uint8_t lastValidState = static_cast<uint8_t>(ATMeInputState::COUNT - 3);

    if (current < lastValidState) {
        inputState = static_cast<ATMeInputState>(current + 1);
    } else {
        inputState = ATMeInputState::INPUT_ADDRESSES;
    }
}

int32_t ATMeController::constrainAddition(int32_t input, int32_t addition, int32_t min, int32_t max, int32_t nearest = 1) {
    int32_t added;
    if (nearest > 1 || nearest < -1) {
        if (addition > 0) {
            added = ((input + addition) / nearest) * (nearest);
        } else{
            added = ((input + addition + 1) / nearest) * (nearest);
        }
    } else {
        added = input + addition;
    }

    if (added < min) return min;
    if (added > max) return max;
    return added;
}

void ATMeController::processControlInputs(int32_t fanDelta, int32_t hazeDelta, bool fanButton, bool hazeButton) {
    int32_t fanMultiplier = 1;
    int32_t hazeMultiplier = 1;
    if (!fanButton) fanMultiplier = 10;
    if (!hazeButton) hazeMultiplier = 10;

    fanDelta *= fanMultiplier;
    hazeDelta *= hazeMultiplier;
    fanValue = uint8_t(constrainAddition(fanValue, fanDelta, 0, 255, fanMultiplier));
    hazeLevel = uint8_t(constrainAddition(hazeLevel, hazeDelta, 0, 255, hazeMultiplier));
}

void ATMeController::saveAddresses() {}
void ATMeController::loadAddresses() {}