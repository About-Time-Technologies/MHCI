/*********
  Complete project details at https://randomnerdtutorials.com
  
  This is an example for our Monochrome OLEDs based on SSD1306 drivers. Pick one up today in the adafruit shop! ------> http://www.adafruit.com/category/63_98
  This example is for a 128x32 pixel display using I2C to communicate 3 pins are required to interface (two I2C and one reset).
  Adafruit invests time and resources providing this open source code, please support Adafruit and open-source hardware by purchasing products from Adafruit!
  Written by Limor Fried/Ladyada for Adafruit Industries, with contributions from the open source community. BSD license, check license.txt for more information All text above, and the splash screen below must be included in any redistribution. 
*********/

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <encoder.hpp>
#include <esp_dmx.h>


Encoder fan(0, 255, 0, 100, EncoderMode::SCALED, 10);
Encoder haze(0, 255, 0, 3000, EncoderMode::SCALED, 10);

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

dmx_port_t dmx_port = 1; // DMX port to use
byte dmxData[DMX_PACKET_SIZE]; // DMX data buffer
unsigned long lastDmxUpdate = millis(); // Last DMX update time


void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN); // SDA, SCL pins for ESP32

  fan.begin(0);
  haze.begin(1);

  dmx_config_t dmx_config = DMX_CONFIG_DEFAULT;
  dmx_personality_t dmx_personalities[] = {};
  uint8_t dmx_personality_count = 0;
  dmx_driver_install(dmx_port, &dmx_config, dmx_personalities, dmx_personality_count);
  dmx_set_pin(dmx_port, TRANSMIT_PIN, RECEIVE_PIN, ENABLE_PIN);

  //SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.display();
}

void updateDisplay(uint8_t fan, float haze) {
  display.clearDisplay();
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.printf("     Fan", fan);
  if( fan < 10) {
    display.setCursor(24,0);             // Start at top-left corner
  } else if (fan < 100) {
    display.setCursor(12,0);             
  } else {
    display.setCursor(0,0);             
  }
  display.printf("%d%%\n", fan);

  display.printf("     PSI", haze);
  display.setCursor(0,16);             // Start at top-left corner
  display.printf("%.2f\n", haze);
  display.display();
}

void sendDMX(uint8_t fan, uint8_t haze) {
  dmxData[1] = fan;
  dmxData[2] = haze;

  dmx_write(dmx_port, dmxData, DMX_PACKET_SIZE); // Write DMX data to the buffer

  dmx_send(dmx_port);
  dmx_wait_sent(dmx_port, DMX_TIMEOUT_TICK); // Wait for DMX data to be sent
}

void loop() {
  fan.update();
  haze.update();

  if(fan.getAndClearStateChanged() || haze.getAndClearStateChanged()) {
    updateDisplay(uint8_t(fan.getMappedValue()), float(haze.getMappedValue()) / 1000.0);
  }

  sendDMX(uint8_t(fan.getRawValue()), uint8_t(haze.getRawValue()));
}