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
    display(_display),
    frontLEDLevel(0),
    rearLEDLevel(0),
    hazeLongPress(false), fanLongPress(false),
    menuTimeout(20000)
    {};

bool ATMeController::begin(unsigned long now) {
    dmx->begin(now);
    display->begin(now);

    fanEncoder.begin(now, 0);
    hazeEncoder.begin(now, 1);

    fanEncoder.onButtonEvent(ButtonEvent::BUTTON_PRESS, []() { ESP_LOGD("Fan", "Press"); } );
    fanEncoder.onButtonEvent(ButtonEvent::BUTTON_DOUBLE_PRESS, [this]() { 
        ESP_LOGD("Fan", "Double press"); 

        this->nextState();
    } );
    fanEncoder.onButtonEvent(ButtonEvent::BUTTON_LONG_PRESS_START, [this]() { 
        ESP_LOGD("Fan", "Long press"); 
        fanLongPress = true;
        if (hazeLongPress) this->inputState = ATMeInputState::INPUT_PURGE_HOLD;
        triggerDisplayUpdate;
    } );
    fanEncoder.onButtonEvent(ButtonEvent::BUTTON_LONG_PRESS_END, [this]() { 
        ESP_LOGD("Fan", "Long press end"); 
        this->fanLongPress = false;
    });

    hazeEncoder.onButtonEvent(ButtonEvent::BUTTON_PRESS, []() { ESP_LOGD("Haze", "Press"); } );
    hazeEncoder.onButtonEvent(ButtonEvent::BUTTON_DOUBLE_PRESS, [this]() {
        ESP_LOGD("Haze", "Double press"); 
        
        this->nextState();
    } );
    hazeEncoder.onButtonEvent(ButtonEvent::BUTTON_LONG_PRESS_START, [this]() { 
        ESP_LOGD("Haze", "Long press"); 
        hazeLongPress = true;
        
        if (fanLongPress) this->inputState = ATMeInputState::INPUT_PURGE_HOLD;
        triggerDisplayUpdate;
    } );
    hazeEncoder.onButtonEvent(ButtonEvent::BUTTON_LONG_PRESS_END, [this]() { 
        ESP_LOGD("Haze", "Long press end"); 
        hazeLongPress = false;
    });

    loadAddresses();

    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);

    ledcAttachPin(PWM1_PIN, PWM_CHANNEL);
    ledcAttachPin(PWM2_PIN, PWM_CHANNEL + 1);

    ledcWrite(PWM_CHANNEL, 0);
    ledcWrite(PWM_CHANNEL+1, 0);

    return true;
}

bool ATMeController::update(unsigned long now) {
    int32_t fanDelta = fanEncoder.update(now);
    int32_t hazeDelta = hazeEncoder.update(now);

    if (fanDelta) ESP_LOGD(TAG, "Fan delta: %d", fanDelta);
    if (hazeDelta) ESP_LOGD(TAG, "Haze delta: %d", hazeDelta);

    hazeOn = true;
    if (ATMeInputState::INPUT_PURGE_ACTIVE == inputState) hazeOn = false;

    if (ATMeInputState::INPUT_ADDRESSES == inputState || ATMeInputState::INPUT_LEDs == inputState) {
        menuTimeout.startIfNotStarted(now);

        if (menuTimeout.checkTimeout(now)) {
            inputState = ATMeInputState::INPUT_CONTROL;
            menuTimeout.stop();
        }
    }

    if (fanDelta || hazeDelta) {

        switch (inputState) {
            case ATMeInputState::INPUT_CONTROL:
                processControlInputs(fanDelta, hazeDelta, fanEncoder.getButtonState(), hazeEncoder.getButtonState());
                break;
            case ATMeInputState::INPUT_ADDRESSES:
                processAddressInputs(fanDelta, hazeDelta, fanEncoder.getButtonState(), hazeEncoder.getButtonState());
                menuTimeout.start(now);
                break;
            case ATMeInputState::INPUT_LEDs:
                processLEDInputs(fanDelta, hazeDelta, fanEncoder.getButtonState(), hazeEncoder.getButtonState());
                menuTimeout.start(now);
                break;
            case ATMeInputState::INPUT_PURGE_HOLD:
                processControlInputs(fanDelta, hazeDelta, fanEncoder.getButtonState(), hazeEncoder.getButtonState());
                break;
            case ATMeInputState::INPUT_PURGE_ACTIVE:
                processControlInputs(fanDelta, hazeDelta, fanEncoder.getButtonState(), hazeEncoder.getButtonState());
                break;
        }

    }

    uint8_t brightness = 128;

    switch (inputState) {
        case ATMeInputState::INPUT_CONTROL:
            if (!hazeEncoder.getButtonState()) {
                hazeEncoder.updateNeopixel(0,brightness,0);
            } else {
                hazeEncoder.updateNeopixel(0,brightness,brightness/4);
            }

            if (!fanEncoder.getButtonState()) {
                fanEncoder.updateNeopixel(0,brightness,0);
            } else {
                fanEncoder.updateNeopixel(0,brightness,brightness/4);
            }
            break;
        case ATMeInputState::INPUT_ADDRESSES:
            hazeEncoder.updateNeopixel(brightness,brightness,0);
            fanEncoder.updateNeopixel(brightness,brightness,0);
            break;
        case ATMeInputState::INPUT_LEDs:
            hazeEncoder.updateNeopixel(0,0,brightness);
            fanEncoder.updateNeopixel(0,0,brightness);
            break;
        case ATMeInputState::INPUT_PURGE_HOLD:
            hazeEncoder.updateNeopixel(brightness,0,brightness);
            fanEncoder.updateNeopixel(brightness,0,brightness);
            break;
        case ATMeInputState::INPUT_PURGE_ACTIVE:
            hazeEncoder.updateNeopixel(brightness,0,0);
            fanEncoder.updateNeopixel(brightness,0,0);
            break;
    }


    if (dmx->update(now, *this)) {
        //ESP_LOGV(TAG, "DMX update successful");
    } else {
        ESP_LOGW(TAG, "DMX update failed");
    }

    if (fanDelta || hazeDelta) triggerDisplayUpdate = true;



    if (triggerDisplayUpdate) {
        triggerDisplayUpdate = false; // Reset the flag after updating the display
        display->update(now, true, *this);

        fanEncoder.updateNeopixel(fanValue,0,0);
        hazeEncoder.updateNeopixel(0,hazeLevel,0);
    }

    display->update(now, false, *this);

    ledcWrite(PWM_CHANNEL, frontLEDLevel);
    ledcWrite(PWM_CHANNEL+1, rearLEDLevel);

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
            return "GLOW";
        case ATMeInputState::INPUT_PURGE_HOLD:
            return "CONFIRM";
        case ATMeInputState::INPUT_PURGE_ACTIVE:
            return "PURGE";
    }
    return "ERR";
}

ATMeInputState ATMeController::getInputState() {
    return inputState;
}

void ATMeController::nextState() {
    uint8_t current = static_cast<uint8_t>(inputState);
    uint8_t lastValidState = static_cast<uint8_t>(ATMeInputState::COUNT - 3);
    uint8_t lastState = static_cast<uint8_t>(ATMeInputState::COUNT - 2);

    if (current < lastValidState || current == lastState) {
        inputState = static_cast<ATMeInputState>(current + 1);
    } else {
        inputState = ATMeInputState::INPUT_CONTROL;
    }

    triggerDisplayUpdate = true;
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

void ATMeController::processAddressInputs(int32_t fanDelta, int32_t hazeDelta, bool fanButton, bool hazeButton) {
    int32_t fanMultiplier = 1;
    int32_t hazeMultiplier = 1;
    
    if (!fanButton) fanMultiplier = 10;
    if (!hazeButton) hazeMultiplier = 10;

    fanDelta *= fanMultiplier;
    hazeDelta *= hazeMultiplier;
    fanAddress = uint16_t(constrainAddition(fanAddress, fanDelta, 1, hazeAddress - 1, fanMultiplier));
    hazeAddress = uint16_t(constrainAddition(hazeAddress, hazeDelta, fanAddress + 1, 510, hazeMultiplier));

    saveAddresses();
}

void ATMeController::processLEDInputs(int32_t fanDelta, int32_t hazeDelta, bool fanButton, bool hazeButton) {
    int32_t fanMultiplier = 1;
    int32_t hazeMultiplier = 1;
    if (!fanButton) fanMultiplier = 10;
    if (!hazeButton) hazeMultiplier = 10;

    fanDelta *= fanMultiplier;
    hazeDelta *= hazeMultiplier;
    frontLEDLevel = uint8_t(constrainAddition(frontLEDLevel, fanDelta, 0, 255, fanMultiplier));
    rearLEDLevel = uint8_t(constrainAddition(rearLEDLevel, hazeDelta, 0, 255, hazeMultiplier));

    saveAddresses();
}

void ATMeController::saveAddresses() {
    preferences.begin("atme", false);

    preferences.putUInt("fanAddress", fanAddress);
    preferences.putUInt("hazeAddress", hazeAddress);

    preferences.putUInt("frontLEDLevel", frontLEDLevel);
    preferences.putUInt("rearLEDLevel", rearLEDLevel);

    preferences.end();
}

void ATMeController::loadAddresses() {
    preferences.begin("atme", true);

    fanAddress = uint16_t(preferences.getUInt("fanAddress", 1));
    hazeAddress = uint16_t(preferences.getUInt("hazeAddress", 2));

    frontLEDLevel = uint16_t(preferences.getUInt("frontLEDLevel", 0));
    rearLEDLevel = uint16_t(preferences.getUInt("rearLEDLevel", 0));

    preferences.end();
}