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

    switch (atmeController.getInputState()) {
        case ATMeInputState::INPUT_LEDs:
            display.setCursor(8,textY);
            display.print("FRONT");

            display.setCursor(rightX,textY);  
            display.print("REAR");

            percentageValue(display, uint8_t(map(atmeController.frontLEDLevel, 0, 255, 0, 100)), leftX, outputY);

            percentageValue(display, uint8_t(map(atmeController.rearLEDLevel, 0, 255, 0, 100)), rightX, outputY);
            break;
        case ATMeInputState::INPUT_ADDRESSES:
            display.setCursor(14,textY);
            display.print("FAN");

            display.setCursor(rightX,textY);  
            display.print("HAZE");

            dmxValue(display, atmeController.fanAddress, 14 , outputY);
            dmxValue(display, atmeController.hazeAddress, rightX + 6, outputY);
            break;
        default:
            display.setCursor(14,textY);
            display.print("FAN");

            display.setCursor(rightX,textY);  
            display.print("HAZE");

            uint8_t fanValue = uint8_t(map(atmeController.fanValue, 0, 255, 0, 100));
            percentageValue(display, fanValue, leftX, outputY);

            uint8_t hazeValue = uint8_t(map(atmeController.hazeLevel, 0, 255, 0, 100));
            percentageValue(display, hazeValue, rightX, outputY);
            break;
    }

    display.setCursor(0,60);
    centreText(display, atmeController.getInputStateString(), 64);
    if (atmeController.getInputStateString() == "CONFIRM") {
        centreText(display, "PURGE", 80);
    } else {
        centreText(display, "MODE", 80);
    }

    uint8_t maxHeight = 35;
    uint8_t leftLength = uint8_t(map(atmeController.frontLEDLevel,0,255,0,maxHeight));
    if (atmeController.frontLEDLevel == 0) leftLength = 0;

    display.fillRect(0, 96, 4, -leftLength, SSD1327_WHITE);



    uint8_t rightLength = uint8_t(map(atmeController.rearLEDLevel,0,255,0,maxHeight));
    if (atmeController.rearLEDLevel == 0) leftLength = 0;

    display.fillRect(SCREEN_HEIGHT - 4, 96, 4, -rightLength, SSD1327_WHITE);

    display.display();

    return true;
}

void ATMeDisplay::centreText(Adafruit_SSD1327 &display, const std::string text, int y) {
    int16_t x = ((128 - text.length() * 12) / 2);
    display.setCursor(x, y);
    display.printf(text.c_str());
}

void ATMeDisplay::percentageValue(Adafruit_SSD1327 &display, const uint16_t percentage, int x, int y) {
    if(percentage < 10) {
        display.setCursor(x + 24,y);             // Start at top-left corner
    } else if (percentage < 100) {
        display.setCursor(x + 12,y);             
    } else {
        display.setCursor(x,y);             
    }
    display.printf("%d%%\n", percentage);
}

void ATMeDisplay::dmxValue(Adafruit_SSD1327 &display, const uint16_t dmx, int x, int y) {
    display.setCursor(x, y);

    if(dmx < 10) {
        display.printf("00%d\n", dmx);
    } else if (dmx < 100) {
        display.printf("0%d\n", dmx);            
    } else {
        display.printf("%d\n", dmx);            
    }
    
}