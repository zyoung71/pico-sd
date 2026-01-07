#include <stdio.h>

#include <storage/SDCardSDIO.h>
#include <storage/SDCardSPI.h>

#include <f_util.h>

void sd_card_detect(const Event* ev, void* user_data)
{
    GPIOEvent* event = ev->GetEventAsType<GPIOEvent>();
    if (event->GetEventsTriggeredMask() & GPIO_IRQ_EDGE_RISE)
        printf("SD Card Connected.\n");
}

int main()
{
    stdio_init_all();

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    puts("Program start.\n");
    
    //SDCardSDIO card(3, 4);
    SDCardSPI card(2, 3, 4, 7);
    
    bool ok = card.Mount();
    printf("Mounted? %d\n", ok);
    
    SDCardDetector sd_detect(6, &card);

    int id = sd_detect.AddAction(&sd_card_detect);

    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    card.OpenFile("file.txt", FA_OPEN_APPEND | FA_WRITE);

    puts("Opened file.\n");

    StorageDeviceStream& stream = card.GetStream();
    stream << "write this to card";
    stream << ' ' << 100;

    puts("Wrote data to card.\n");

    card.CloseFile();

    constexpr size_t chunk_size = 32;
    char buff[chunk_size];
    size_t total_read_size = 0;

    gpio_put(PICO_DEFAULT_LED_PIN, 0);

    puts("Beginning Loop...\n");

    while (1)
    {
        tight_loop_contents();
    }

    return 0;
}