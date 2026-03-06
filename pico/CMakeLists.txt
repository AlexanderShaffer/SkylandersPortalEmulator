# This file is part of Skylanders Portal Emulator.
# Copyright (C) 2026  Alexander Shaffer <alexander.shaffer.623@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 4.2.3)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(PICO_BOARD pico_w)
include(FetchContent)

# FetchContent_Populate() must be used instead of FetchContent_MakeAvailable() to avoid adding the PicoSDK to the build.
FetchContent_Populate(PicoSdk
    GIT_REPOSITORY https://github.com/raspberrypi/pico-sdk.git
    GIT_TAG a1438dff1d38bd9c65dbd693f0e5db4b9ae91779 # Release 2.2.0
    GIT_SUBMODULES_RECURSE FALSE
)

include(${picosdk_SOURCE_DIR}/pico_sdk_init.cmake)
project(SkylandersPortalEmulator)
pico_sdk_init()

FetchContent_Declare(FreeRtos
    GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
    GIT_TAG 0adc196d4bd52a2d91102b525b0aafc1e14a2386 # Release 11.2.0
)

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM INTERFACE config/)
target_compile_definitions(freertos_config INTERFACE projCOVERAGE_TEST=0)
set(FREERTOS_PORT "GCC_RP2040")
FetchContent_MakeAvailable(FreeRtos)
include(${FETCHCONTENT_BASE_DIR}/freertos-src/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

add_executable(SkylandersPortalEmulator config/UsbDescriptors.cpp src/Main.cpp)
target_include_directories(SkylandersPortalEmulator PRIVATE config/ ../common/)
target_link_libraries(SkylandersPortalEmulator PRIVATE pico_stdlib tinyusb_device tinyusb_board pico_cyw43_arch_lwip_sys_freertos FreeRTOS-Kernel-Heap4)
target_compile_definitions(SkylandersPortalEmulator PRIVATE NO_SYS=0 WIFI_SSID=${WIFI_SSID} WIFI_PASSWORD=${WIFI_PASSWORD} HOST_IPV4_ADDRESS=${HOST_IPV4_ADDRESS})
pico_add_extra_outputs(SkylandersPortalEmulator)