#include <storage/SDCardSPI.hpp>

SDCardSPI::SDCardSPI(const SDCardSPI::Pinout& pins, spi_inst_t* spi_inst, const char* pc_name)
    : SDCard(pc_name), SPIDevice(pins.clk_pin, pins.mosi_pin, pins.miso_pin, pins.cs_pin)
{
    spi.hw_inst = spi_inst;
    spi.sck_gpio = pins.clk_pin;
    spi.mosi_gpio = pins.mosi_pin;
    spi.miso_gpio = pins.miso_pin;
    spi.baud_rate = baud_rate;

    card_interface.spi = &spi;
    card_interface.ss_gpio = pins.cs_pin;
    
    card.type = SD_IF_SPI;
    card.spi_if_p = &card_interface;
}

SDCardSPI::SDCardSPI(uint8_t clk_pin, uint8_t mosi_pin, uint8_t miso_pin, uint8_t cs_pin, spi_inst_t* spi_inst, const char* pc_name)
    : SDCard(pc_name), SPIDevice(clk_pin, mosi_pin, miso_pin, cs_pin)
{
    spi.hw_inst = spi_inst;
    spi.sck_gpio = clk_pin;
    spi.mosi_gpio = mosi_pin;
    spi.miso_gpio = miso_pin;
    spi.baud_rate = baud_rate;

    card_interface.spi = &spi;
    card_interface.ss_gpio = cs_pin;
    
    card.type = SD_IF_SPI;
    card.spi_if_p = &card_interface;
}