#include "ATMeDMX/atmedmx.hpp"

#include <string>

bool ATMeDMX::begin(unsigned long now) {
  dmx_config_t dmx_config = DMX_CONFIG_DEFAULT;
  dmx_personality_t dmx_personalities[] = {};
  uint8_t dmx_personality_count = 0;

  bool success = true;

  success &= dmx_driver_install(dmx_port, &dmx_config, dmx_personalities, dmx_personality_count);

  if (!success) {
      ESP_LOGE(TAG, "Failed to initialise DMX driver");
      return success;
  }
  success &= dmx_set_pin(dmx_port, TRANSMIT_PIN, RECEIVE_PIN, ENABLE_PIN);

  if (!success) {
      ESP_LOGE(TAG, "Failed to initialise DMX IO");
      return success;
  }

  ESP_LOGI(TAG, "Successfully initialised DMX driver");

  rdmUpdate.start(millis());
  dmxUpdate.start(millis());

  return success;
}

bool ATMeDMX::update(unsigned long now, uint16_t _fanAddress, uint8_t _fanValue, uint16_t _hazeAddress, bool _unitOn, uint8_t _hazeValue, bool _hazeOn) {
  fan.address = _fanAddress;
  fan.value = _fanValue;

  assert(_hazeAddress <= 510);

  unitOn.address = _hazeAddress;
  unitOn.value = _unitOn ? 255 : 0;
  
  hazeLevel.address = _hazeAddress + 1;
  hazeLevel.value = _hazeValue;

  hazeOn.address = _hazeAddress + 2;
  hazeOn.value = _hazeOn ? 255 : 0;

  if (rdmUpdate.checkTimeoutAndRestart(now)) {
    hazerStateString = updateHazerStateString();
    ESP_LOGD(TAG, "Hazer state: %s", hazerStateString.c_str());
  }

  if (dmxUpdate.checkTimeoutAndRestart(now)) {
    return sendDMX();
  }

  return true;
}

bool ATMeDMX::sendDMX() {
  dmxData[fan.address] = fan.value;
  dmxData[unitOn.address] = unitOn.value;
  dmxData[hazeLevel.address] = hazeLevel.value;
  dmxData[hazeOn.address] = hazeOn.value;

  dmx_write(dmx_port, dmxData, DMX_PACKET_SIZE); // Write DMX data to the buffer

  dmx_send(dmx_port);
  dmx_wait_sent(dmx_port, DMX_TIMEOUT_TICK); // Wait for DMX data to be sent


  return true;
}

std::string ATMeDMX::getHazerStateString() {
  return hazerStateString;
}

std::string ATMeDMX::updateHazerStateString() {
  int devicesFound = rdm_discover_devices_simple(dmx_port, uids, 32);

  /* If any devices were found during discovery, lets iterate through them. */
  for (int i = 0; i < devicesFound; ++i) {
    ESP_LOGI(TAG, "Device %i has UID " UIDSTR "\n", i, UID2STR(uids[i]));

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
        ESP_LOGW(TAG, "Unknown device found");
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
        ESP_LOGE(TAG, UIDSTR " GET DMX_START_ADDRESS NACK reason: 0x%02x\n",
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
        ESP_LOGW(TAG, UIDSTR " GET GENERATOR_STATE NACK reason: 0x%02x\n",
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
    return "MDG N/A";
  }

  return "ERROR";
}