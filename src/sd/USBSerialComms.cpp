#include <sd/USBSerialComms.h>

#include <comms/serial_usb.h>

namespace comms
{

    size_t sd_card_to_serial_usb_blocking(SDCardStream& stream, size_t chunk_size, uint32_t timeout_ms, const SerialUSBDetector* detector)
    {
        char buff_data[chunk_size];
        stream.ExtractBuffer(buff_data, chunk_size);
        return comms_serial_write_buff_over_usb_blocking(buff_data, chunk_size, timeout_ms);
    }

    size_t sd_card_to_serial_usb_nonblocking(SDCardStream& stream, uint32_t chunk_size)
    {
        char buff_data[chunk_size];
        stream.ExtractBuffer(buff_data, chunk_size);
        return comms_serial_try_write_buff_over_usb(buff_data, chunk_size);
    }

    size_t serial_usb_to_sd_card_blocking(SDCardStream& stream, size_t chunk_size, uint32_t timeout_ms, const SerialUSBDetector* detector)
    {
        char buff[chunk_size];
        size_t bytes_read = 0;
        if (comms_serial_read_buff_over_usb_blocking(buff, chunk_size, timeout_ms, &bytes_read) == COMMS_OK)
        {
            stream << buff;
        }
        return bytes_read;
    }

    size_t serial_usb_to_sd_card_nonblocking(SDCardStream& stream, size_t chunk_size)
    {
        char buff[chunk_size];
        size_t bytes_read = 0;
        if (comms_serial_try_read_buff_over_usb(buff, chunk_size, &bytes_read) != COMMS_FAIL)
        {
            stream << buff;
        }
        return bytes_read;
    }
}