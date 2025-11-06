#include "ATMeDisplay/atmedisplay.hpp"
#include "ATMeController/atmecontroller.hpp"

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

bool ATMeDisplay::begin(unsigned long now) {
    //SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(0x3D, true)) {
        //if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        ESP_LOGW(this->TAG, "SSD1306 allocation failed");
        //for(;;); // Don't proceed, loop forever
    }

    display.setRotation(0);
    display.clearDisplay();
    display.display();

    displayUpdate.start(now);
    flashingDisplayTimeout.start(now);

    ESP_LOGD(this->TAG, "Initialised");
    return true;
};

bool ATMeDisplay::update(unsigned long now, bool forceUpdate, ATMeController& atmeController, bool atmeAlert, bool controlAlert) {
    if (!forceUpdate && !displayUpdate.checkTimeoutAndRestart(now)) return false;

    
    displayUpdate.start(now);
    display.clearDisplay();
    display.setTextSize(2);             // Normal 1:1 pixel scale


    uint8_t outputY = 42;
    uint8_t textY = 22;
    uint8_t lineWidth = 4;
    uint8_t leftX = 6;
    uint8_t rightX = 74;

    if (flashingDisplayTimeout.checkTimeoutAndRestart(now)) {
        if (flashingBackgroundColour) {
            flashingBackgroundColour = false;
        } else {
            flashingBackgroundColour = true;
        }
        ESP_LOGD(TAG, "Toggling background colour, %d", flashingBackgroundColour);
    }

    if (atmeAlert) {
        if (flashingBackgroundColour) {
            drawATMeStateGraphics(display, atmeController, SSD1327_WHITE, SSD1327_BLACK);
        } else {
            drawATMeStateGraphics(display, atmeController, SSD1327_BLACK, SSD1327_WHITE);
        }
    } else {
        drawATMeStateGraphics(display, atmeController, SSD1327_WHITE, SSD1327_BLACK);
    }

    if (controlAlert) {
        if (flashingBackgroundColour) {
            drawControlStateGraphics(display, atmeController, SSD1327_WHITE, SSD1327_BLACK);
        } else {
            drawControlStateGraphics(display, atmeController, SSD1327_BLACK, SSD1327_WHITE);
        }
    } else {
        drawControlStateGraphics(display, atmeController, SSD1327_WHITE, SSD1327_BLACK);
    }

    display.setTextColor(SSD1327_WHITE); // Draw white text
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
        case ATMeInputState::INPUT_CONTROL:
            display.setCursor(14,textY + 10);
            display.print("FAN");

            display.setCursor(rightX,textY + 10);  
            display.print("HAZE");

            uint8_t fanValue = uint8_t(map(atmeController.fanValue, 0, 255, 0, 100));
            percentageValue(display, fanValue, leftX, outputY + 10);

            uint8_t hazeValue = uint8_t(map(atmeController.hazeLevel, 0, 255, 0, 100));
            percentageValue(display, hazeValue, rightX, outputY + 10);
            break;
    }

    display.setCursor(0,60);
    drawCentredText(display, atmeController.getInputStateString(), 60);

    if (atmeController.unitOn) {
        display.fillRect(0, 78, 2, 2, SSD1327_WHITE);
    }

    if (atmeController.hazeOn) {
        display.fillRect(128-2, 78, 2, 2, SSD1327_WHITE);
    }
    
    // uint8_t maxHeight = 35;

    // if (atmeController.frontLEDLevel > 0) {
    //     uint8_t leftLength = uint8_t(map(atmeController.frontLEDLevel,0,255,0,maxHeight));

    //     display.fillRect(0, 80, 4, -leftLength, SSD1327_WHITE);
    // }

    // if (atmeController.rearLEDLevel > 0) {
    //     uint8_t rightLength = uint8_t(map(atmeController.rearLEDLevel,0,255,0,maxHeight));

    //     display.fillRect(SCREEN_HEIGHT - 4, 80, 4, -rightLength, SSD1327_WHITE);
    // }

    display.display();

    return true;
}

void ATMeDisplay::drawATMeStateGraphics(Adafruit_SSD1327 &display, ATMeController& atmeController, uint16_t backgroundColour, uint16_t textColour) {
    display.fillRect(0, 0, SCREEN_HEIGHT, 16, backgroundColour); // Draw a border around the text

    display.setTextColor(textColour); 

    drawCentredText(display, atmeController.getATMeStateString(), 1); // Center the generator state text
}

void ATMeDisplay::drawControlStateGraphics(Adafruit_SSD1327 &display, ATMeController& atmeController, uint16_t backgroundColour, uint16_t textColour) {
    display.setTextColor(textColour); 

    display.fillRect(0, 96-16, SCREEN_HEIGHT, 16, backgroundColour); // Draw a border around the text
    
    drawCentredText(display, atmeController.getControlStateString(), 96-15); // Center the generator state text
}

void ATMeDisplay::drawCentredText(Adafruit_SSD1327 &display, const std::string text, int y) {
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