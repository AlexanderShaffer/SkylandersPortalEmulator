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

cmake_minimum_required(VERSION 4.4.3)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(
    DOWNLOAD
    https://raw.githubusercontent.com/raspberrypi/pico-sdk/079c6f39023649b154152db30f1d781e884879bc/external/pico_sdk_import.cmake
    ${CMAKE_BINARY_DIR}/pico_sdk_import.cmake
)

set(PICO_SDK_FETCH_FROM_GIT ON)
set(PICO_SDK_FETCH_FROM_GIT_TAG 079c6f39023649b154152db30f1d781e884879bc)
set(PICO_BOARD pico_w)

include(${CMAKE_BINARY_DIR}/pico_sdk_import.cmake)
project(SkylandersPortalEmulator)
pico_sdk_init()

add_executable(SkylandersPortalEmulator src/Main.cpp src/UsbDescriptors.cpp)
target_include_directories(SkylandersPortalEmulator PRIVATE config/ ../common/)
target_link_libraries(SkylandersPortalEmulator pico_stdlib pico_btstack_ble pico_btstack_cyw43 pico_cyw43_arch_none tinyusb_device tinyusb_board pico_multicore)
pico_btstack_make_gatt_header(SkylandersPortalEmulator PRIVATE ${CMAKE_CURRENT_LIST_DIR}/config/server.gatt)
pico_add_extra_outputs(SkylandersPortalEmulator)