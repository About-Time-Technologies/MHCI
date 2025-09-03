#include "virtualbutton.hpp"
#include <Arduino.h>

VirtualButton::VirtualButton(unsigned long debounce, unsigned long longPress, unsigned long doubleClick) :
        debounceTime(debounce), longPressTime(longPress), doubleClickTime(doubleClick),
        lastState(false), currentState(false), lastChangeTime(0),
        pressStartTime(0), lastReleaseTime(0), longPressTriggered(false), doubleClickWaiting(false) {

    for (int i = 0; i < 8; ++i) {
        callbacks[i] = nullptr;
    }
}

void VirtualButton::on(ButtonEvent event, ButtonCallback callback) {
    callbacks[event] = callback;
}

void VirtualButton::trigger(ButtonEvent event) {
    if (callbacks[event]) {
        callbacks[event]();
    }
}

void VirtualButton::update(unsigned long currentTime, bool isPressed) {
    currentState = isPressed;

 

    if (currentState != lastState && (currentTime - lastChangeTime) > debounceTime) {
        // State change, and we've exceeded the debounce time
        lastChangeTime = currentTime;

        if (currentState) {
            pressStartTime = currentTime;
            longPressTriggered = false;
            trigger(BUTTON_PRESS_DOWN);
        } else {
            trigger(BUTTON_PRESS_UP);

            if (!longPressTriggered) {
                trigger(BUTTON_PRESS);

                if (doubleClickWaiting && (currentTime - lastReleaseTime) < doubleClickTime) {
                    trigger(BUTTON_DOUBLE_PRESS);
                    doubleClickWaiting = false;
                } else {
                    doubleClickWaiting = true;
                    lastReleaseTime = currentTime;
                }
            } else {
                trigger(BUTTON_LONG_PRESS_END);
            }
        }
        lastState = currentState;
    }


    if (currentState && !longPressTriggered && (currentTime - pressStartTime) > longPressTime) {
        longPressTriggered = true;
        trigger(BUTTON_LONG_PRESS_START);
    }

 

    if (currentState && longPressTriggered) {
        trigger(BUTTON_LONG_PRESS_HOLD);
    }
}

bool VirtualButton::getState() {
    return currentState;
}