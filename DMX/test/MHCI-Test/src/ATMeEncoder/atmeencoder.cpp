#include "ATMeEncoder/atmeencoder.hpp"


ATMeEncoder::ATMeEncoder() :
    TAG("ATMeEncoder"),
    buttonState(false),
    hardwareEncoder(Adafruit_seesaw()),
    hardwareNeopixel(seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800)) {

    for (int i = 0; i < 1; i++) {
        callbacks[i] = nullptr;
    }
}

bool ATMeEncoder::begin(unsigned long now, const uint8_t addressOffset) {
    ESP_LOGV(TAG, "Initialising encoder #%d", addressOffset);

    if (!hardwareEncoder.begin(SEESAW_BASE_ADDR + addressOffset) || !hardwareNeopixel.begin(SEESAW_BASE_ADDR + addressOffset)) {
        ESP_LOGE(TAG, "Couldn't find encoder #%d", addressOffset);
        return found;
    } 

    ESP_LOGD(TAG, "Found encoder + pixel #%d", addressOffset);

    uint32_t version = ((hardwareEncoder.getVersion() >> 16) & 0xFFFF);
    if (version != 4991) {
        ESP_LOGW(TAG, "Wrong firmware loaded: %d", version);
        return found;
    }

    // Configure the encoder and neopixel
    hardwareEncoder.pinMode(SS_SWITCH, INPUT_PULLUP);
    hardwareEncoder.setGPIOInterrupts((uint32_t)1 << SS_SWITCH, 1);
    hardwareEncoder.enableEncoderInterrupt();
    hardwareNeopixel.setBrightness(30);
    hardwareNeopixel.show();

    found = true;

    ESP_LOGD(TAG, "Encoder #%d initialised", addressOffset);
    return found;
}

bool ATMeEncoder::update(unsigned long now) {
    if (!found) return false;

    hardwareButton.update(now, !hardwareEncoder.digitalRead(SS_SWITCH));

    int32_t encoderDelta = hardwareEncoder.getEncoderDelta();

    if (encoderDelta) trigger(EncoderEvent::ENCODER_DELTA, encoderDelta);

    return true;
}

bool ATMeEncoder::updateNeopixel(const uint8_t r, const uint8_t g, const uint8_t b) {
    if (!found) return false;

    // Update the neopixel color
    hardwareNeopixel.setPixelColor(0, r, g, b);
    hardwareNeopixel.show();

    return true;
}

void ATMeEncoder::onButtonEvent(ButtonEvent event, ButtonCallback callback) {
    hardwareButton.on(event, callback);
}

void ATMeEncoder::onEncoderEvent(EncoderEvent event, EncoderCallback callback) {
    callbacks[event] = callback;
}

void ATMeEncoder::trigger(EncoderEvent event, int32_t delta) {
    if (callbacks[event]) {
        callbacks[event](delta);
    }
}