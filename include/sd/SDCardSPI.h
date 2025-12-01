#pragma once

#include "SDCard.h"

class SDCardSPI : public SDCard
{
public:
    struct Pinout
    {
        const uint8_t clk_pin;
        const uint8_t miso_pin;
        const uint8_t mosi_pin;
        const uint8_t cs_pin;
    };

    static constexpr uint32_t baud_rate = 125 * 1000 * 1000 / 4;

private:
    Pinout pins;

    sd_spi_if_t card_interface;
    spi_t spi;

public:
    SDCardSPI(const SDCardSPI::Pinout& pins, spi_inst_t* spi_inst = spi0, const char* pc_name = "0:");
    SDCardSPI(uint8_t clk_pin, uint8_t miso_pin, uint8_t mosi_pin, uint8_t cs_pin, spi_inst_t* spi_inst = spi0, const char* pc_name = "0:");
    ~SDCardSPI() = default;

};