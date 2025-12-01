if (NOT TARGET pico-sd)

    add_library(pico-sd STATIC
        src/sd/SDCard.cpp
        src/sd/SDCardSDIO.cpp
        src/sd/SDCardSPI.cpp
        src/sd/USBSerialComms.cpp
    )

    target_include_directories(pico-sd PUBLIC
        include
    )

    add_subdirectory(lib/pico-event-hardware)
    add_subdirectory(lib/pico-fatfs/src)

    target_link_libraries(pico-sd
        pico-event-hardware
        no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
    )

endif()