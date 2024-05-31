cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(gps_interposer)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

if (TARGET tinyusb_device)
    # rest of your project
    add_executable(gps_interposer
        gps_interposer.cc
        ../shared/m12_parser.h
        ../shared/WS2812.cpp
    )

    pico_generate_pio_header(gps_interposer ${CMAKE_CURRENT_LIST_DIR}/../shared/WS2812.pio)

    # enable usb output, disable uart output
    pico_enable_stdio_usb(gps_interposer 1)
    pico_enable_stdio_uart(gps_interposer 0)
    
    # Add pico_stdlib library which aggregates commonly used features
    target_link_libraries(gps_interposer 
        pico_stdlib
        hardware_pio
    )
    
    # create map/bin/hex/uf2 file in addition to ELF.
    pico_add_extra_outputs(gps_interposer)

elseif(PICO_ON_DEVICE)
    message(WARNING "not building hello_usb because TinyUSB submodule is not initialized in the SDK")
endif()

