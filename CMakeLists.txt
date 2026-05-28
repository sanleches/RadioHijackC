# =============================================================================
# RadioHijackC - Pico SDK Build Configuration
# =============================================================================
#
# Board:  Raspberry Pi Pico 2 W (RP2350)
# Stack:  Pico SDK 2.2.0 + CYW43 Wi-Fi + lwIP raw API
#
# Purpose:
#   Configure, build, and package the RadioHijackC firmware.
#
# Build responsibilities:
#   Select C/C++ standards, import Pico SDK, set PICO_BOARD, declare all source
#   modules, enable USB serial, link hardware/Wi-Fi/lwIP libraries, and generate
#   deployable UF2/BIN/HEX outputs.
#
# Key output:
#   build/RadioHijackC.uf2
# =============================================================================

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(RadioHijackC C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(RadioHijackC
        src/main.cpp
        src/net/api_router.cpp
        src/net/dhcp_server.cpp
        src/net/http_response.cpp
        src/net/web_server.cpp
        src/net/wifi_manager.cpp
        src/radio/rda5807m.cpp
        src/radio/rds_parser.cpp
        src/storage/preset_store.cpp
        src/ui/serial_console.cpp
        src/ui/status_led.cpp
        src/ui/web_page.cpp
        src/util/logger.cpp
        src/util/url.cpp
)

pico_set_program_name(RadioHijackC "RadioHijackC")
pico_set_program_version(RadioHijackC "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(RadioHijackC 0)
pico_enable_stdio_usb(RadioHijackC 1)

# Standalone-power safety: never wait for a USB host and never block firmware
# progress trying to write serial logs when powered from a USB charger.
target_compile_definitions(RadioHijackC PRIVATE
        PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=0
        PICO_STDIO_USB_STDOUT_TIMEOUT_US=0
)

# Add the standard library to the build
target_link_libraries(RadioHijackC
        pico_stdlib
        hardware_flash
        hardware_i2c
        hardware_sync)

# Add the standard include files to the build
target_include_directories(RadioHijackC PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

# Add any user requested libraries
target_link_libraries(RadioHijackC 
        pico_cyw43_arch_lwip_threadsafe_background
        )

pico_add_extra_outputs(RadioHijackC)
