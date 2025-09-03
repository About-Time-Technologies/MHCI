#pragma once
#include <stdint.h>

class DMXFixture {
public:
    DMXFixture(uint8_t _address = 1, uint8_t _value = 0) : address(_address), value(_value) {};
    uint16_t address;
    uint8_t value;
};