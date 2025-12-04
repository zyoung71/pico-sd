#include <stdio.h>

#include <sd/SDCardSDIO.h>
#include <sd/SDCardSPI.h>
#include <sd/USBSerialComms.h>

void sd_card_detect(const Event* ev, void* user_data)
{
    printf("SD Card Connected.\n");
}

int main()
{
    //SDCardSDIO card(3, 4);
    SDCardSPI card(2, 3, 4, 5);
    SDCardDetector sd_detect(0, &card);
    
    void (*func_ptr)(const Event*, void*) = &sd_card_detect;
    sd_detect.SetActions(&func_ptr, 1);

    card.Mount();
    card.OpenFile("file.txt", FA_OPEN_ALWAYS | FA_READ | FA_WRITE);

    SDCardStream& stream = card.GetStream();
    stream << "write this to card";
    stream << ' ' << 100;

    card.CloseFile();

    constexpr size_t chunk_size = 32;
    char buff[chunk_size];
    size_t total_read_size = 0;
    while (1)
    {
        total_read_size += comms::serial_usb_to_sd_card_nonblocking(stream, chunk_size);
    }

    return 0;
}