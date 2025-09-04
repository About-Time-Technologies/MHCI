#include "ATMeDisplay/atmedisplay.hpp"
#include "ATMeController/atmecontroller.hpp"

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

bool ATMeDisplay::begin(unsigned long now) {
    //SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(0x3C, true)) {
        //if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        ESP_LOGW(this->TAG, "SSD1306 allocation failed");
        for(;;); // Don't proceed, loop forever
    }

    display.setRotation(0);
    display.clearDisplay();
    display.display();

    displayUpdate.start(now);
    flashingDisplay.start(now);

    ESP_LOGD(this->TAG, "Initialised");
    return true;
};

bool ATMeDisplay::update(unsigned long now, bool forceUpdate, ATMeController& atmeController) {
    if (!forceUpdate && !displayUpdate.checkTimeoutAndRestart(now)) return false;

    
    displayUpdate.start(now);
    display.clearDisplay();
    display.setTextSize(2);             // Normal 1:1 pixel scale


    uint8_t outputY = 42;
    uint8_t textY = 22;
    uint8_t lineWidth = 4;
    uint8_t leftX = 6;
    uint8_t rightX = 74;

    uint16_t topBackgroundColor = SSD1327_WHITE;
    uint16_t topTextColor = SSD1327_BLACK;

    bool errorState = false;
    if (errorState && flashingDisplay.checkTimeoutAndRestart(millis())) {
        displayInvert = !displayInvert; // Toggle the display invert state
    }

    if (displayInvert) {
    topBackgroundColor = SSD1327_BLACK;
    topTextColor = SSD1327_WHITE;
    }

    display.fillRect(0, 0, SCREEN_HEIGHT, 16, topBackgroundColor); // Draw a border around the display
    //display.fillRect(SCREEN_HEIGHT/2 - lineWidth/2, 0, lineWidth, SCREEN_WIDTH, SH110X_WHITE); 
    display.setTextColor(topTextColor);        // Draw white text
    centreText(display, atmeController.getATMeStateString(), 1); // Center the generator state text

    display.setTextColor(SSD1327_WHITE);        // Draw white text

    display.setCursor(14,textY);
    display.print("FAN");

    uint8_t fanValue = uint8_t(map(atmeController.fanValue, 0, 255, 0, 100));

    if(fanValue < 10) {
        display.setCursor(leftX + 24,outputY);             // Start at top-left corner
    } else if (fanValue < 100) {
        display.setCursor(leftX + 12,outputY);             
    } else {
        display.setCursor(leftX,outputY);             
    }
    display.printf("%d%%\n", fanValue);

    display.setCursor(rightX,textY);  
    display.print("HAZE");

    display.setCursor(rightX,outputY);             // Start at top-left corner

    uint8_t hazeValue = uint8_t(map(atmeController.hazeLevel, 0, 255, 0, 100));
    if(hazeValue < 10) {
        display.setCursor(rightX + 24,outputY);             // Start at top-left corner
    } else if (hazeValue < 100) {
        display.setCursor(rightX + 12,outputY);             
    } else {
        display.setCursor(rightX,outputY);             
    }
    display.printf("%d%%\n", hazeValue);

    display.setCursor(0,60);

    centreText(display, atmeController.getInputStateString(), 68);

    display.display();

    return true;
}

void ATMeDisplay::centreText(Adafruit_SSD1327 &display, const std::string text, int y) {
    int16_t x = ((128 - text.length() * 12) / 2);
    display.setCursor(x, y);
    display.printf(text.c_str());
}