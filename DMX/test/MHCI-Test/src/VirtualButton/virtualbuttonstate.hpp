
#include <unordered_map>
#include <functional>
#include "virtualbutton.hpp"


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

class VirtualButtonState {
public:
    using Callback = std::function<void()>;

    VirtualButtonState() : startTime(0) {}
    virtual ~VirtualButtonState() {}

    // Called when entering the state
    virtual void onEnter(VirtualButtonState* previousState, unsigned long currentTime) {
        startTime = currentTime;
    }

    // Called when input is received
    virtual void handleInput(VirtualButton& virtualButton, unsigned long currentTime, bool isPressed) {}

    // Called periodically to update state
    virtual void update(VirtualButton& virtualButton, unsigned long currentTime) {}

    // Called when exiting the state — returns the next state
    virtual VirtualButtonState* exitState() = 0;

    // Register a callback for an event
    void on(ButtonEvent event, Callback callback) {
        callbacks[event] = callback;
    }

    // Trigger an event
    void trigger(ButtonEvent event) {
        if (!callbacks.count(event)) {
            if (callbacks.count(ButtonEvent::BUTTON_ERROR)) {
                callbacks[ButtonEvent::BUTTON_ERROR]();
            }
        }

        callbacks[event]();
    }

    // Accessor for global click count
    static unsigned int getClickCount() {
        return clickCount;
    }

    static void incrementClickCount() {
        ++clickCount;
    }

    static void resetClickCount() {
        clickCount = 0;
    }

protected:
    unsigned long startTime;
    std::unordered_map<ButtonEvent, Callback> callbacks;

private:
    static unsigned int clickCount;
};

// Definition of static member
unsigned int VirtualButtonState::clickCount = 0;


class NoState : public VirtualButtonState {

public:

    void onEnter(VirtualButtonState* previousState, unsigned long currentTime) override {
        Serial.println("Entering NoState");
        VirtualButtonState::onEnter(previousState, currentTime);
        resetClickCount();
        trigger(ButtonEvent::BUTTON_PRESS_END);
    }

    void handleInput(VirtualButton& virtualButton, unsigned long currentTime, bool isPressed) override {
        if (isPressed) {
            return new NoState();
        }
    }
};