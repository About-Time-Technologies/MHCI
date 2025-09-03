#pragma once
#include <Timeout/timeout.hpp>
#include <functional>
#include <unordered_map>
#include "virtualbuttonstate.hpp"

enum ButtonEvent {
    BUTTON_PRESS_DOWN,
    BUTTON_PRESS_UP,
    BUTTON_PRESS_END,
    BUTTON_PRESS_REPEAT_DONE,
    BUTTON_PRESS_SINGLE_CLICK,
    BUTTON_PRESS_REPEAT,
    BUTTON_DOUBLE_CLICK,
    BUTTON_LONG_PRESS_START,
    BUTTON_LONG_PRESS_HOLD,
    BUTTON_LONG_PRESS_END,
    BUTTON_ERROR,
};

class VirtualButton {
  public:
    using Callback = std::function<void()>;

    VirtualButton() : state(VirtualButton_NoState()) {}

    void on(ButtonEvent event, Callback callback) {
        callbacks[event] = callback;
    }

    void trigger(ButtonEvent event) {
        if (!callbacks.count(event)) {
            if (callbacks.count(BUTTON_ERROR)) {
                callbacks[BUTTON_ERROR]();
            }
        }

        callbacks[event]();
    }

    void setState(bool newHardwareState, unsigned long currentTime) {
        if (hardwareState == newHardwareState) return; // No change in hardware state           
        hardwareState = newHardwareState;
        stateFlag = true;

        if (hardwareState) {
            trigger(BUTTON_PRESS_DOWN);
        } else {
            trigger(BUTTON_PRESS_UP);
        }

        return;
    }

    bool getHardwareState() const {
        return hardwareState;
    }

    void update(unsigned long currentTime) {
        if (!stateFlag) return;
        stateFlag = false;
    }
    

  private:
    VirtualButtonState state;
    bool hardwareState = false;
    bool stateFlag = false;
    
    std::unordered_map<ButtonEvent, Callback> callbacks;
}