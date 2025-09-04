#include "ATMeEncoder/atmeencoder.hpp"


ATMeEncoder::ATMeEncoder() :
    TAG("ATMeEncoder"),
    buttonState(false),
    hardwareEncoder(Adafruit_seesaw()),
    hardwareNeopixel(seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800)) {

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

int32_t ATMeEncoder::update(unsigned long now) {
    if (!found) return false;

    buttonState = !hardwareEncoder.digitalRead(SS_SWITCH);

    hardwareButton.update(now, buttonState);

    return hardwareEncoder.getEncoderDelta();
}

bool ATMeEncoder::updateNeopixel(const uint8_t r, const uint8_t g, const uint8_t b) {
    if (!found) return false;

    // Update the neopixel color
    hardwareNeopixel.setPixelColor(0, r, g, b);

    return true;
}

bool ATMeEncoder::showNeopixel() {
    hardwareNeopixel.show();
    return true;
}

void ATMeEncoder::onButtonEvent(ButtonEvent event, ButtonCallback callback) {
    hardwareButton.on(event, callback);
}