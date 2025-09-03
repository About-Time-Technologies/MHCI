#include "ATMeDisplay/atmedisplay.hpp"

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

bool ATMeDisplay::update(unsigned long now, bool forceUpdate, uint8_t fan, float haze, std::string generatorState, uint8_t hazeValue, bool unitOn, bool hazeOn) {
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
    centreText(display, generatorState, 1); // Center the generator state text

    display.setTextColor(SSD1327_WHITE);        // Draw white text

    display.setCursor(14,textY);
    display.print("FAN");


    if( fan < 10) {
        display.setCursor(leftX + 24,outputY);             // Start at top-left corner
    } else if (fan < 100) {
        display.setCursor(leftX + 12,outputY);             
    } else {
        display.setCursor(leftX,outputY);             
    }
    display.printf("%d%%\n", fan);

    display.setCursor(rightX,textY);  
    display.print("HAZE");
    display.setCursor(rightX,outputY);             // Start at top-left corner
    display.printf("%.2f\n", haze);

    // display.setCursor(0,48);
    // display.printf("L: %d", hazeValue);

    // display.setCursor(88,48);
    // display.printf("U: %s", unitOn ? "ON" : "OFF");
    // display.setCursor(88, 24);
    // display.printf("H: %s", hazeOn ? "ON" : "OFF");

    display.display();

    return true;
}

void ATMeDisplay::centreText(Adafruit_SSD1327 &display, const std::string text, int y) {
    int16_t x = ((128 - text.length() * 12) / 2);
    display.setCursor(x, y);
    display.printf(text.c_str());
}