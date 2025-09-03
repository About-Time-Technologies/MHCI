#pragma once
#include <Timeout/timeout.hpp>
#include <functional>
#include <unordered_map>

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

    VirtualButton() : state(false) {}

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

    void setState(bool newState, unsigned long currentTime) {
        if (state == newState) return; // No change in state           
        state = newState;
        stateChanged = true;

        if (state) {
            trigger(BUTTON_PRESS_DOWN);
        } else {
            trigger(BUTTON_PRESS_UP);
        }

        return;
    }

    bool getState() const {
        return state;
    }

    void update(unsigned long currentTime) {
        if (!stateChanged) return;
        stateChanged = false;
    }
    

  private:
    bool state;
    bool stateChanged = false;
    
    std::unordered_map<ButtonEvent, Callback> callbacks;
}