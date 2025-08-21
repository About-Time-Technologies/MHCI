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
#include <string>
#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
//#include <Adafruit_SH110X.h>
#include <Adafruit_SSD1327.h>
#include <encoder.hpp>
#include <esp_dmx.h>
#include <dmx/include/service.h>
#include <rdm/controller.h>
#include <rdm/controller/include/utils.h>
#include <Timeout/timeout.hpp>

Timeout rdmUpdate(5000);
Timeout displayUpdate(250);
bool displayUpdateRequired = false;
std::string hazerState = "\n";

bool hazeOn = false;
bool unitOn = false;

Encoder fan(0, 255, 0, 100, EncoderMode::SCALED, 10);
Encoder haze(0, 255, 0, 3000, EncoderMode::SCALED, 10);


Timeout flashingDisplay(500);
bool displayInvert = false;

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
//Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET, 1000000, 100000);
Adafruit_SSD1327 display(128, 128, &Wire, OLED_RESET, 1000000);
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
dmx_port_t dmx_port = 1; // DMX port to use
byte dmxData[DMX_PACKET_SIZE]; // DMX data buffer
unsigned long lastDmxUpdate = millis(); // Last DMX update time

rdm_uid_t uids[32];
rdm_uid_t hazerUID;

size_t rdm_send_get_generator_state(dmx_port_t dmx_num,
                                    const rdm_uid_t *dest_uid,
                                    rdm_sub_device_t sub_device,
                                    char (&response)[20],
                                    rdm_ack_t *ack){

  DMX_CHECK(dmx_num < DMX_NUM_MAX, 0, "dmx_num error");
  DMX_CHECK(dest_uid != NULL, 0, "dest_uid is null");
  DMX_CHECK(sub_device < RDM_SUB_DEVICE_MAX, 0, "sub_device error");
  DMX_CHECK(dmx_driver_is_installed(dmx_num), 0, "driver is not installed");

  const rdm_request_t request = {
    .dest_uid = dest_uid, 
    .sub_device = sub_device,
    .cc = RDM_CC_GET_COMMAND,
    .pid = 32770,
  };

  const char *format = "bbbbbbbbbbbbbbbbbbbb$";

  return rdm_send_request(dmx_num, &request, format, response,
                          sizeof(*response), ack);
}

std::string getHazerStateString() {
  int devicesFound = rdm_discover_devices_simple(dmx_port, uids, 32);

  /* If any devices were found during discovery, lets iterate through them. */
  for (int i = 0; i < devicesFound; ++i) {
    Serial.printf("Device %i has UID " UIDSTR "\n", i, UID2STR(uids[i]));

    /* Now we will send RDM requests to the devices we found. We first need to
      address our requests to the proper device and sub-device. We will get a
      pointer to one of the UIDs we got in discovery to properly address our RDM
      requests. We will address our requests to the root RDM sub device. We will
      also declare an RDM ACK to get information about RDM responses, but this
      isn't necessary if it is not desired. */
    rdm_uid_t destUID = uids[i];
    rdm_sub_device_t subDevice = RDM_SUB_DEVICE_ROOT;
    rdm_ack_t ack;

    /* First, we will send a request to get the device information of our RDM
      device. We can pass our DMX port and pointers to our header, device info,
      and ACK to our request function. If the request is successful, we will
      print out some of the device information we received. */
    rdm_device_info_t deviceInfo;
    if (rdm_send_get_device_info(dmx_port, &destUID, subDevice, &deviceInfo,
                                 &ack)) {
      //Serial.printf(
      //    "Model ID: %i, DMX Footprint: %i, Sub-device count: %i, Sensor count: %i\n",
      //    deviceInfo.model_id, deviceInfo.footprint, deviceInfo.sub_device_count,
      //    deviceInfo.sensor_count);
    }

    if (deviceInfo.model_id == 5 && destUID.man_id == 0x4d44) {
      //Serial.println("ATMe found!");
    } else {
      Serial.println("Unknown device found!");
      continue; // Skip to the next device
    }

    /* Finally, we will get and set the DMX start address. It is not required
      for all RDM devices to support the DMX start address parameter so it is
      possible (but unlikely) that your device does not support this parameter.
      After getting the DMX start address, we will increment it by one. */
    uint16_t dmxStartAddress;
    if (rdm_send_get_dmx_start_address(dmx_port, &destUID, subDevice,
                                       &dmxStartAddress, &ack)) {
      //Serial.printf("DMX start address is %i\n", dmxStartAddress);


    } else if (ack.type == RDM_RESPONSE_TYPE_NACK_REASON) {
      /* In the event that the DMX start address request fails, print the reason
        for the failure. The NACK reason and other information on the response
        can be found in the ack variable we declared earlier. */
      Serial.printf(UIDSTR " GET DMX_START_ADDRESS NACK reason: 0x%02x\n",
                    ack.src_uid, ack.nack_reason);
    }

    char generatorResponse[20] = {0};
    std::string generatorResponseString = "";

    uint8_t firstNull = 0;

    if (rdm_send_get_generator_state(dmx_port, &destUID, subDevice,
                                     generatorResponse, &ack)) {
      //generatorResponseString = std::string(generatorResponse);
      //Serial.printf("Found generator state: %s\n", generatorResponseString.c_str());

      for (int i = 0; i < 20; i++) {
        if (generatorResponse[i] == '\0') {
          firstNull = i;
          break; // Stop at the first null character
        }
      }

      for (int i = 0; i < firstNull - 2; i++) {
        generatorResponseString += generatorResponse[i];
      }

    } else if (ack.type == RDM_RESPONSE_TYPE_NACK_REASON) {
      Serial.printf(UIDSTR " GET GENERATOR_STATE NACK reason: 0x%02x\n",
                    ack.src_uid, ack.nack_reason);

      generatorResponseString = "NACK";
    }

    Serial.println(generatorResponseString.back());

    if (generatorResponseString.find("OFF") != std::string::npos) {
      // If the generator state is OFF, we can return early
      return "HAZE OFF";
    }

    if (generatorResponseString.find("HEAT") != std::string::npos) {
      // If the generator state is OFF, we can return early
      
      std::string heatString = "HEAT: ";
      if (isDigit(generatorResponseString.at(0))) {
        heatString += generatorResponseString.at(0);
      } else {
        heatString += "0"; // Default to 0 if the first character is not a digit
      }

      if (isDigit(generatorResponseString.at(1))) {
        heatString += generatorResponseString.at(1);
      } else {
        heatString += "0"; // Default to 0 if the second character is not a digit
      }

      heatString += "%%";

      return heatString;
    }

    if (generatorResponseString.find("PURGE") != std::string::npos) {
      // If the generator state is OFF, we can return early
      return "PURGING";
    }

    if (generatorResponseString.find("READY") != std::string::npos) {
      // If the generator state is OFF, we can return early
      return "HAZE READY";
    }

    if (generatorResponseString.find("ON") != std::string::npos) {
      // If the generator state is OFF, we can return early
      return "HAZE ON";
    }

    if (generatorResponseString.find("FAIL") != std::string::npos) {
      // If the generator state is OFF, we can return early
      return "MDG ERROR";
    }

    return generatorResponseString; // Return the generator state as a string
  }

  if (devicesFound == 0) {
    /* Oops! No RDM devices were found. Double-check your DMX connections and
      try again. */
    //Serial.printf("Could not find any RDM capable devices.\n");
    return "MDG OFF";
  }

  return "ERROR";
}


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
  if(!display.begin(0x3C, true)) {
  //if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.setRotation(0);
  display.clearDisplay();
  display.display();

  rdmUpdate.start(millis());

  displayUpdate.start(millis());

  flashingDisplay.start(millis());
}

void centreText(Adafruit_SSD1327 &display, const std::string text, int y) {
  int16_t x = ((128 - text.length() * 12) / 2);
  display.setCursor(x, y);
  display.printf(text.c_str());
}

void updateDisplay(uint8_t fan, float haze, std::string generatorState, uint8_t hazeValue, bool unitOn = false, bool hazeOn = false) {
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
}

void sendDMX(uint8_t fan, uint8_t haze, bool unitOn, bool hazeOn) {
  dmxData[1] = fan;
  dmxData[2] = unitOn ? 255 : 0; // If unitOn is true, set to 255, otherwise 0
  dmxData[3] = haze;
  dmxData[4] = hazeOn ? 255 : 0; // If hazeOn is true, set to 255, otherwise 0

  dmx_write(dmx_port, dmxData, DMX_PACKET_SIZE); // Write DMX data to the buffer

  dmx_send(dmx_port);
  dmx_wait_sent(dmx_port, DMX_TIMEOUT_TICK); // Wait for DMX data to be sent
}

void loop() {
  fan.update();
  haze.update();

  switch (fan.getButtonEvent()) {
    case ButtonEvent::BUTTON_CLICKED:
      Serial.println("Fan button clicked");
      unitOn = !unitOn; // Toggle unit on/off
      break;
    case ButtonEvent::BUTTON_LONG_PRESS_START:
      Serial.println("Fan button long press started");
      break;
    case ButtonEvent::BUTTON_NONE:
      break;
    default:
      break;
  }

  switch (haze.getButtonEvent()) {
    case ButtonEvent::BUTTON_CLICKED:
      Serial.println("Haze button clicked");
      hazeOn = !hazeOn; // Toggle haze on/off
      break;
    case ButtonEvent::BUTTON_LONG_PRESS_START:
      Serial.println("Haze button long press started");
      break;
    case ButtonEvent::BUTTON_NONE:
      break;
    default:
      break;
  }


  if (rdmUpdate.checkTimeoutAndRestart(millis())) {
    hazerState = getHazerStateString();
    Serial.printf("Hazer state: %s\n", hazerState.c_str());
    displayUpdateRequired = true; // Set the flag to update the display
  }

  if (fan.getAndClearEncoderState() || haze.getAndClearEncoderState()) {
    displayUpdateRequired = true; // Set the flag to update the display
  }
  

  if (displayUpdate.checkTimeoutAndRestart(millis()) || displayUpdateRequired) {
    Serial.println("Updating display...");
    displayUpdateRequired = false; // Reset the flag after updating the display
    updateDisplay(uint8_t(fan.getMappedValue()), float(haze.getMappedValue()) / 1000.0, hazerState, uint8_t(haze.getRawValue()), unitOn, hazeOn);
  }

  sendDMX(uint8_t(fan.getRawValue()), uint8_t(haze.getRawValue()), unitOn, hazeOn);
}