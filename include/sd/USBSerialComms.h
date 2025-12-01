#pragma once

#include "SDCard.h"
#include <hardware/SerialUSB.h>

namespace comms
{

    size_t sd_card_to_serial_usb_blocking(SDCardStream& stream, size_t chunk_size, uint32_t timeout_ms, const SerialUSBDetector* detector = serial_usb_detector_instance);
    size_t sd_card_to_serial_usb_nonblocking(SDCardStream& stream, size_t chunk_size);

    size_t serial_usb_to_sd_card_blocking(SDCardStream& stream, size_t chunk_size, uint32_t timeout_ms, const SerialUSBDetector* detector = serial_usb_detector_instance);
    size_t serial_usb_to_sd_card_nonblocking(SDCardStream& stream, size_t chunk_size);

}