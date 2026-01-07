#pragma once

#include "SDCard.h"

class SDCardSDIO : public SDCard
{
public:
    // You MUST reserve pins D0 - 2, D0 + 1, D0 + 2, D0 + 3 on top of the two here.
    struct Pinout
    {
        const uint8_t cmd_pin;
        const uint8_t d0_pin;
        const uint8_t d1_pin = d0_pin + 1;
        const uint8_t d2_pin = d0_pin + 2;
        const uint8_t d3_pin = d0_pin + 3;
        const uint8_t clk_pin = d0_pin - 2;
    };

    static constexpr uint32_t baud_rate = 125 * 1000 * 1000 / 6;

private:
    Pinout pins;

    sd_sdio_if_t card_interface;

public:
    SDCardSDIO(const SDCardSDIO::Pinout& pins, const char* pc_name = "");
    SDCardSDIO(uint8_t cmd_pin, uint8_t d0_pin, const char* pc_name = "");
    ~SDCardSDIO() = default;
};