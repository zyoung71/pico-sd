if (NOT TARGET pico-sd)

    add_library(pico-sd STATIC
        src/storage/SDCard.cpp
        src/storage/SDCardSDIO.cpp
        src/storage/SDCardSPI.cpp
    )

    target_include_directories(pico-sd PUBLIC
        include
    )

    add_subdirectory(lib/pico-storage-device)
    add_subdirectory(lib/pico-event-hardware)
    add_subdirectory(lib/pico-fatfs/src)

    target_link_libraries(pico-sd
        pico-storage-device
        pico-event-hardware
        no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
    )
    
endif()