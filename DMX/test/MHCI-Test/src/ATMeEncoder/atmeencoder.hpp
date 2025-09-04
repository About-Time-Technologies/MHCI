#pragma once

#define LOG_LOCAL_LEVEL LOG_LEVEL
#include "esp_log.h"

#include <Adafruit_seesaw.h>
#include <seesaw_neopixel.h>
#include <VirtualButton/virtualbutton.hpp>


#define SS_SWITCH 24
#define SS_NEOPIX 6
#define SEESAW_BASE_ADDR          0x36

enum EncoderEvent {
    ENCODER_DELTA
};

typedef void (*EncoderCallback)(int32_t delta);

class ATMeEncoder {
public:
    ATMeEncoder();

    bool begin(unsigned long now, const uint8_t addressOffset);
    bool update(unsigned long now);

    void onButtonEvent(ButtonEvent event, ButtonCallback callback);
    void onEncoderEvent(EncoderEvent event, EncoderCallback callback);

    bool getButtonState();

    bool updateNeopixel(const uint8_t r, const uint8_t g, const uint8_t b);

private: 
    const char* TAG;

    Adafruit_seesaw hardwareEncoder;
    seesaw_NeoPixel hardwareNeopixel;
    VirtualButton hardwareButton;
    bool found = false;

    
    bool buttonState;

    EncoderCallback callbacks[1];
    void trigger(EncoderEvent event, int32_t delta);
};