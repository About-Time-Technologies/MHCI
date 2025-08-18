#pragma once
#include <Timeout/timeout.hpp>

enum ButtonEvent {
    BUTTON_CLICKED,
    BUTTON_LONG_PRESS_START,
    BUTTON_NONE,
};

class VirtualButton {
  public:
    VirtualButton() : state(false) {}

    void setState(bool newState, unsigned long currentTime) {
        if (state == newState) return; // No change in state           
        state = newState;
        stateChanged = true;

        // Button has been released.
        if (!this->state) {
            return;
        }

        // Button has been pressed down.
        //Serial.println("Button pressed, starting timeouts");
        longPressTimeout.start(currentTime);
        clickTimeout.start(currentTime);

        return;
    }

    bool getState() const {
        return state;
    }

    void update(unsigned long currentTime) {
        if (!stateChanged) return;
        stateChanged = false;

        //Serial.println("Button state changed to: " + String(state));

        // We check to see if the button was pressed long enough to trigger a long press.
        // if (!state && longPressTimeout.checkTimeoutNoStop(currentTime)) {
        //     // If the long press timeout is active and it has timed out, we return a long press event.
        //     longPressTimeout.stop(); // Stop the long press timeout
        //     clickTimeout.stop(); // Stop the click timeout
        //     lastEvent = BUTTON_LONG_PRESS_START;
        //     Serial.println("Button long press started");
        //     return;
        // }

        if (!state && clickTimeout.isActive() && !clickTimeout.checkTimeoutNoStop(currentTime)) {
            // If the click timeout is active and it has timed out, we return a click event.
            longPressTimeout.stop();
            clickTimeout.stop(); // Stop the click timeout
            lastEvent = BUTTON_CLICKED;
            Serial.println("Button clicked");
            return;
        }
    }

    ButtonEvent getLastEvent() {
        // If neither timeout is active, we reset the last event.
        ButtonEvent tempEvent = lastEvent;
        lastEvent = BUTTON_NONE;
        return tempEvent;
    }
    

  private:
    bool state;
    bool stateChanged = false;
    ButtonEvent lastEvent = BUTTON_NONE;

    Timeout longPressTimeout{1000}; // 1 second for long press
    Timeout clickTimeout{500}; // <500 ms for single click
};