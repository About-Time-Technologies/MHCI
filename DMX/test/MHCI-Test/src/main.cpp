/*********
  Complete project details at https://randomnerdtutorials.com
  
  This is an example for our Monochrome OLEDs based on SSD1306 drivers. Pick one up today in the adafruit shop! ------> http://www.adafruit.com/category/63_98
  This example is for a 128x32 pixel display using I2C to communicate 3 pins are required to interface (two I2C and one reset).
  Adafruit invests time and resources providing this open source code, please support Adafruit and open-source hardware by purchasing products from Adafruit!
  Written by Limor Fried/Ladyada for Adafruit Industries, with contributions from the open source community. BSD license, check license.txt for more information All text above, and the splash screen below must be included in any redistribution. 
*********/
#include <Arduino.h>
#include <Wire.h>
#include "ATMeController/atmecontroller.hpp"
#include "ATMeDisplay/atmedisplay.hpp"
#include "ATMeDMX/atmedmx.hpp"

ATMeController* atme;
ATMeDisplay display;
ATMeDMX dmx;

void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN); // SDA, SCL pins for ESP32

  atme = new ATMeController(&display, &dmx);

  atme->begin(millis());
}

void loop() {
  atme->update(millis());
}