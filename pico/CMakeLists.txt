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
set(PICO_SDK_FETCH_FROM_GIT ON)
include(pico_sdk_import.cmake)

project(SkylandersPortalEmulator)
pico_sdk_init()

set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_LIST_DIR}/dependencies/freertos)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

add_executable(SkylandersPortalEmulator config/UsbDescriptors.cpp src/Main.cpp)

target_include_directories(SkylandersPortalEmulator PRIVATE config/ dependencies/freertos/include/ ../common/)
target_link_libraries(SkylandersPortalEmulator PRIVATE pico_stdlib pico_unique_id tinyusb_device tinyusb_board pico_cyw43_arch_lwip_sys_freertos pico_lwip_iperf FreeRTOS-Kernel-Heap4)
target_compile_definitions(SkylandersPortalEmulator PRIVATE NO_SYS=0 WIFI_SSID=${WIFI_SSID} WIFI_PASSWORD=${WIFI_PASSWORD} HOST_IPV4_ADDRESS=${HOST_IPV4_ADDRESS})
pico_add_extra_outputs(SkylandersPortalEmulator)