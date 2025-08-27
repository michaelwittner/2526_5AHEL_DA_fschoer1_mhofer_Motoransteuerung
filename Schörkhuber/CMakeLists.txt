# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set name of project (as PROJECT_NAME) and C/C++ standards
project(blink C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# Tell CMake where to find the executable source file
add_executable(${PROJECT_NAME} 
    main.c
)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(${PROJECT_NAME} 
    pico_stdlib
)

# PICO_CONFIG: PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE, Enable/disable resetting into BOOTSEL mode via an additional VENDOR USB interface
# enables picotool based reset, type=bool, default=1, group=pico_stdio_usb
target_compile_definitions(${PROJECT_NAME}
    PRIVATE PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE=0
)

# Enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)
