#pragma once
#include <Arduino.h>
#include <Adafruit_seesaw.h>
#include <seesaw_neopixel.h>
#include <VirtualButton/virtualbutton.hpp>


#define SS_SWITCH 24
#define SS_NEOPIX 6
#define SEESAW_BASE_ADDR          0x36

enum EncoderMode {
    RAW,
    BOUNDED,
    SCALED,
};

class Encoder {
  public:
    
    void onPressDown() {
        Serial.println("Encoder button pressed");
    }

    Encoder(const int32_t inputMin, const int32_t inputMax, const int32_t outputMin, const int32_t outputMax, const EncoderMode mode, const uint8_t switchScale = 1) 
      : inputMin(inputMin), inputMax(inputMax), outputMin(outputMin), outputMax(outputMax), mode(mode), switchScale(switchScale) {
        hardwareEncoder = Adafruit_seesaw();
        hardwareNeopixel = seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800);
        found = false;

        encoderButton.on(ButtonEvent::BUTTON_PRESS_DOWN, []() { Serial.println("Button press down"); } );
        encoderButton.on(ButtonEvent::BUTTON_PRESS_UP, []() { Serial.println("Button press up"); } );
        encoderButton.on(ButtonEvent::BUTTON_PRESS_END, []() { Serial.println("Button press end"); } );
        encoderButton.on(ButtonEvent::BUTTON_PRESS_REPEAT_DONE, []() { Serial.println("Button press repeat done"); } );
        encoderButton.on(ButtonEvent::BUTTON_PRESS_SINGLE_CLICK, []() { Serial.println("Button single click"); } );
        encoderButton.on(ButtonEvent::BUTTON_PRESS_REPEAT, []() { Serial.println("Button press repeat"); } );
        encoderButton.on(ButtonEvent::BUTTON_DOUBLE_CLICK, []() { Serial.println("Button double click"); } );
        encoderButton.on(ButtonEvent::BUTTON_LONG_PRESS_START, []() { Serial.println("Button long press start"); } );
        encoderButton.on(ButtonEvent::BUTTON_LONG_PRESS_HOLD, []() { Serial.println("Button long press hold"); } );
        encoderButton.on(ButtonEvent::BUTTON_LONG_PRESS_END, []() { Serial.println("Button long press end"); } );
        encoderButton.on(ButtonEvent::BUTTON_ERROR, []() { Serial.println("Button error"); } );
    }

    bool begin(const uint8_t addressOffset) {
        if (!hardwareEncoder.begin(SEESAW_BASE_ADDR + addressOffset) || !hardwareNeopixel.begin(SEESAW_BASE_ADDR + addressOffset)) {
            Serial.printf("Couldn't find encoder #%d\n", addressOffset);
            return found;
        } 

        Serial.printf("Found encoder + pixel #%d\n", addressOffset);

        uint32_t version = ((hardwareEncoder.getVersion() >> 16) & 0xFFFF);
        if (version != 4991) {
            Serial.printf("Wrong firmware loaded: %d\n", version);
            return found;
        }

        // Configure the encoder and neopixel
        hardwareEncoder.pinMode(SS_SWITCH, INPUT_PULLUP);
        hardwareEncoder.setGPIOInterrupts((uint32_t)1 << SS_SWITCH, 1);
        hardwareEncoder.enableEncoderInterrupt();
        hardwareNeopixel.setBrightness(30);
        hardwareNeopixel.show();

        found = true;
        return found;
    }

    bool update() {
        if (!found) return false;

        // Check for encoder switch press
        encoderButton.setState(!hardwareEncoder.digitalRead(SS_SWITCH), millis()); // Invert the switch state for button logic
  
        // Read the encoder value and delta
        
        int32_t oldEncoderDelta = encoderDelta;
        encoderDelta = hardwareEncoder.getEncoderDelta();
        if (encoderDelta != oldEncoderDelta) { encoderStateChanged = true; } // State changed if the encoder delta is different from the last read

        if (!encoderButton.getState()) {
            encoderDelta *= switchScale; // Scale the delta by the switch scale factor
        }
        encoderVal += encoderDelta;

        if (mode == EncoderMode::RAW) return true;

        encoderVal = constrain(encoderVal, inputMin, inputMax);
        
        return true;
    }

    bool updateNeopixel(const uint8_t r, const uint8_t g, const uint8_t b) {
        if (!found) return false;

        // Update the neopixel color
        hardwareNeopixel.setPixelColor(0, r, g, b);
        hardwareNeopixel.show();

        return true;
    }

    int32_t getMappedValue() {

        switch (mode)
        {
        case EncoderMode::RAW:
            return encoderVal;
            break;
        
        case EncoderMode::BOUNDED:
            return constrain(encoderVal, inputMin, inputMax);
        
        case EncoderMode::SCALED:
            return map(encoderVal, inputMin, inputMax, outputMin, outputMax);
            break;

        default:
            return -1;
            break;
        }
    }

    int32_t getRawValue() {
        return encoderVal;
    }

    bool getAndClearEncoderState() {
        bool state = encoderStateChanged;
        encoderStateChanged = false;
        return state;
    }

    bool getButtonState() const {
        return encoderButton.getState();
    }

private:
    Adafruit_seesaw hardwareEncoder;
    seesaw_NeoPixel hardwareNeopixel;
    bool found = false;

    int32_t encoderVal = 0;
    int32_t encoderDelta = 0;
    uint8_t switchScale;

    EncoderMode mode;

    int32_t inputMin;
    int32_t inputMax;

    int32_t outputMin;
    int32_t outputMax;

    bool encoderStateChanged = false;

    VirtualButton encoderButton;
};
