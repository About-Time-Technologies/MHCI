#pragma once

#define LOG_LOCAL_LEVEL LOG_LEVEL
#include "esp_log.h"

#include <functional>

enum ButtonEvent {
    BUTTON_PRESS_DOWN,
    BUTTON_PRESS_UP,
    BUTTON_PRESS,
    BUTTON_DOUBLE_PRESS,
    BUTTON_LONG_PRESS_START,
    BUTTON_LONG_PRESS_HOLD,
    BUTTON_LONG_PRESS_END
};

//typedef void (*ButtonCallback)();
typedef std::function<void()> ButtonCallback;

class VirtualButton {
public:
    VirtualButton(unsigned long debounceTime = 50, unsigned long longPressTime = 1000, unsigned long doubleClickTime = 500);

    void update(unsigned long currentTime, bool isPressed);

    void on(ButtonEvent event, ButtonCallback callback);

    bool getState();

private:
    const char* TAG;
    bool lastState;
    bool currentState;
    unsigned long lastChangeTime;
    unsigned long pressStartTime;
    unsigned long lastReleaseTime;
    bool longPressTriggered;
    bool doubleClickWaiting;

    bool singleClickPending;
    unsigned singleClickStartTime;

    unsigned long debounceTime;
    unsigned long longPressTime;
    unsigned long doubleClickTime;

    ButtonCallback callbacks[8];

    void trigger(ButtonEvent event);
};