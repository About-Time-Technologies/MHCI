#pragma once

#define LOG_LOCAL_LEVEL LOG_LEVEL
#include "esp_log.h"

#include <Arduino.h>
#include <string>
#include <esp_dmx.h>
#include <dmx/include/service.h>
#include <rdm/controller.h>
#include <rdm/controller/include/utils.h>
#include "DMXFixture/dmxfixture.hpp"
#include "Timeout/timeout.hpp"
#include "ATMeController/atmecontroller.hpp"

class ATMeController;

class ATMeDMX {
public:
    ATMeDMX(uint8_t fanAddress = 1, uint8_t hazerAddress = 2) : 
        dmx_port(1), lastDmxUpdate(millis()), 
        TAG("ATMeDMX"), 
        rdmUpdate(5000), dmxUpdate(25),
        fan(fanAddress, 0), unitOn(hazerAddress,1),
        hazeLevel(hazerAddress + 1, 0), hazeOn(hazerAddress + 2, 0) {};

    bool begin(unsigned long now);
    bool update(unsigned long now, ATMeController& atmeController);

    bool sendDMX();

    std::string getHazerStateString();


private:
    const char* TAG;

    Timeout rdmUpdate;
    Timeout dmxUpdate;

    DMXFixture fan;
    DMXFixture unitOn;
    DMXFixture hazeLevel;
    DMXFixture hazeOn;

    dmx_port_t dmx_port; // DMX port to use
    byte dmxData[DMX_PACKET_SIZE]; // DMX data buffer
    unsigned long lastDmxUpdate; // Last DMX update time


    rdm_uid_t uids[32];
    rdm_uid_t hazerUID;
    bool hazerFound = false;

    bool findHazer();

    std::string hazerStateString = "BOOTING";
    
    std::string updateHazerStateString();

    size_t rdm_send_get_generator_state(dmx_port_t dmx_num,
                                        const rdm_uid_t *dest_uid,
                                        rdm_sub_device_t sub_device,
                                        char (&response)[20],
                                        rdm_ack_t *ack) {

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
};