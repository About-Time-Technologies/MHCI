#include "ATMeController/atmecontroller.hpp"

ATMeController::ATMeController() :
    TAG("ATMeController"),
    fan(0, 255, 0, 100, EncoderMode::SCALED, 10),
    haze(0, 255, 0, 3000, EncoderMode::SCALED, 10) {};

bool ATMeController::begin(unsigned long now) {
    dmx.begin(now);
    display.begin(now);

    fan.begin(0);
    haze.begin(1);

    return true;
}

bool ATMeController::update(unsigned long now) {
    fan.update();
    haze.update();
    dmx.update(now, 1, uint8_t(haze.getRawValue()), 2, unitOn, uint8_t(haze.getRawValue()), hazeOn);

    if (fan.getAndClearEncoderState() || haze.getAndClearEncoderState()) {
        triggerDisplayUpdate = true; // Set the flag to update the display
    }


    if (triggerDisplayUpdate) {
        triggerDisplayUpdate = false; // Reset the flag after updating the display
        display.update(now, true, uint8_t(fan.getMappedValue()), float(haze.getMappedValue()) / 1000.0, dmx.getHazerStateString(), uint8_t(haze.getRawValue()), unitOn, hazeOn);

        fan.updateNeopixel(fan.getMappedValue(),0,0);
        haze.updateNeopixel(0,255,0);
    }

    display.update(now, false, uint8_t(fan.getMappedValue()), float(haze.getMappedValue()) / 1000.0, dmx.getHazerStateString(), uint8_t(haze.getRawValue()), unitOn, hazeOn);





    return true;
}