cmake_minimum_required(VERSION 3.5)

# Using C17 and C++20 by default.
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

# Comment these lines accordingly based on the board you are using!
set(PICO_BOARD pico)
# set(PICO_BOARD pico2)
# set(PICO_BOARD pico_w)
# set(PICO_BOARD pico2_w)

# Below is how we determine which board we are using for both our code, and the compiler.
if (PICO_BOARD STREQUAL "pico")
add_compile_definitions(USING_PICO)
set(PICO_PLATFORM rp2040)
set(PICO_LABEL RPI-RP2)
elseif(PICO_BOARD STREQUAL "pico_w")
add_compile_definitions(USING_PICO_W)
set(PICO_PLATFORM rp2040)
set(PICO_LABEL RPI-RP2)
elseif(PICO_BOARD STREQUAL "pico2")
add_compile_definitions(USING_PICO_2)
set(PICO_PLATFORM rp2350)
set(PICO_LABEL RP2350)
elseif(PICO_BOARD STREQUAL "pico2_w")
add_compile_definitions(USING_PICO_2_W)
set(PICO_PLATFORM rp2350)
set(PICO_LABEL RP2350)
endif()

include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

# Change the first argument with your project name!
project(pico-sd-impl C CXX ASM)

pico_sdk_init()

add_subdirectory(../ build)

add_executable(${CMAKE_PROJECT_NAME} src/main.cpp)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
    include
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    pico-sd
)

pico_enable_stdio_usb(${CMAKE_PROJECT_NAME} 1)
pico_enable_stdio_uart(${CMAKE_PROJECT_NAME} 1)
pico_add_extra_outputs(${CMAKE_PROJECT_NAME})