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
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD d0edc3af-4c50-42ea-a356-e2862fe7a444)
set(CMAKE_CXX_MODULE_STD ON)

project(SkylandersPortalEmulator)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
add_subdirectory(dependencies/sdl3/ EXCLUDE_FROM_ALL)

set(DEAR_IMGUI_PATH dependencies/imgui)

add_library(DearImGui STATIC
    ${DEAR_IMGUI_PATH}/imgui.cpp
    ${DEAR_IMGUI_PATH}/imgui_draw.cpp
    ${DEAR_IMGUI_PATH}/imgui_impl_opengl3.cpp
    ${DEAR_IMGUI_PATH}/imgui_impl_sdl3.cpp
    ${DEAR_IMGUI_PATH}/imgui_tables.cpp
    ${DEAR_IMGUI_PATH}/imgui_widgets.cpp
)

target_include_directories(DearImGui PRIVATE dependencies/sdl3/include/)

add_library(StbImage STATIC dependencies/stb_image/stb_image.cpp)

add_executable(SkylandersPortalEmulator WIN32)

target_sources(SkylandersPortalEmulator
    PRIVATE
    src/Figure.cpp
    src/FigureLoader.cpp
    src/Main.cpp
    src/Owner.cpp
    src/PortalEmulator.cpp
    src/PortalSlot.cpp
    src/Texture.cpp

    PRIVATE FILE_SET CXX_MODULES FILES
    src/Figure.cppm
    src/FigureLoader.cppm
    src/Font.cppm src/Owner.cppm
    src/PortalEmulator.cppm
    src/PortalSlot.cppm
    src/Texture.cppm
)

target_compile_options(SkylandersPortalEmulator PRIVATE -Wno-experimental-header-units)
target_include_directories(SkylandersPortalEmulator PRIVATE dependencies/ ../common/ dependencies/sdl3/include/)
target_link_libraries(SkylandersPortalEmulator PRIVATE SDL3::SDL3 DearImGui StbImage ws2_32.lib OpenGl32.lib -static)
target_compile_definitions(SkylandersPortalEmulator PRIVATE PICO_IPV4_ADDRESS=${PICO_IPV4_ADDRESS})

include(HeaderUnitSupport.cmake)

target_header_units(SkylandersPortalEmulator PRIVATE
    ABSOLUTE_PATH "${CMAKE_CURRENT_LIST_DIR}/../common/Common.hpp"
    ABSOLUTE_PATH "${CMAKE_CURRENT_LIST_DIR}/dependencies/glew/glew.h"
    ABSOLUTE_PATH "${CMAKE_CURRENT_LIST_DIR}/dependencies/imgui/imgui.h"
    ABSOLUTE_PATH "${CMAKE_CURRENT_LIST_DIR}/dependencies/imgui/imgui_impl_opengl3.h"
    ABSOLUTE_PATH "${CMAKE_CURRENT_LIST_DIR}/dependencies/imgui/imgui_impl_sdl3.h"
    ABSOLUTE_PATH "${CMAKE_CURRENT_LIST_DIR}/dependencies/imgui/imgui_internal.h"
    INCLUDE_PATH "${CMAKE_CURRENT_LIST_DIR}/dependencies/sdl3/include" RELATIVE_PATH SDL3/SDL.h
    ABSOLUTE_PATH "${CMAKE_CURRENT_LIST_DIR}/dependencies/stb_image/stb_image.h"
    ABSOLUTE_PATH "${WINSOCK_HEADER_PATH}/winsock2.h"
    ABSOLUTE_PATH "${WINSOCK_HEADER_PATH}/ws2tcpip.h"
)