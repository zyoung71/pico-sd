#include <storage/SDCardSDIO.h>

SDCardSDIO::SDCardSDIO(const SDCardSDIO::Pinout& pins, const char* pc_name)
    : SDCard(pc_name)
{
    card_interface.CLK_gpio = pins.clk_pin;
    card_interface.CMD_gpio = pins.cmd_pin;
    card_interface.D0_gpio = pins.d0_pin;
    card_interface.D1_gpio = pins.d1_pin;
    card_interface.D2_gpio = pins.d2_pin;
    card_interface.D3_gpio = pins.d3_pin;
    card_interface.baud_rate = baud_rate;

    card.type = SD_IF_SDIO;
    card.sdio_if_p = &card_interface;
}

SDCardSDIO::SDCardSDIO(uint8_t cmd_pin, uint8_t d0_pin, const char* pc_name)
    : SDCardSDIO(Pinout{cmd_pin, d0_pin}, pc_name)
{
}