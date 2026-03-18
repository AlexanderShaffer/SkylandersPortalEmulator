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
set(CMAKE_CXX_EXTENSIONS OFF) # Required to avoid a CMake error
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD d0edc3af-4c50-42ea-a356-e2862fe7a444)
set(CMAKE_CXX_MODULE_STD ON)

project(SkylandersPortalEmulator)

include(FetchContent)

FetchContent_Declare(ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG 6ded5230d043aa32c755e65c910c2af5002fb9f9 # Release 1.92.6
)

FetchContent_Declare(Sdl
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG 683181b47cfabd293e3ea409f838915b8297a4fd # Release 3.4.2
)

FetchContent_Declare(Stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 013ac3beddff3dbffafd5177e7972067cd2b5083 # Release 2.30
)

FetchContent_MakeAvailable(ImGui Sdl Stb)
set(IMGUI_PATH "${FETCHCONTENT_BASE_DIR}/imgui-src")
set(SDL_PATH "${FETCHCONTENT_BASE_DIR}/sdl-src")
set(STB_IMAGE_PATH "${FETCHCONTENT_BASE_DIR}/stb-src")

add_library(ImGui STATIC
    ${IMGUI_PATH}/imgui.cpp
    ${IMGUI_PATH}/imgui_draw.cpp
    ${IMGUI_PATH}/imgui_tables.cpp
    ${IMGUI_PATH}/imgui_widgets.cpp
    ${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_PATH}/backends/imgui_impl_sdl3.cpp
)

set(SDL_INCLUDE_OPTION "-I${SDL_PATH}/include")

set(IMGUI_COMPILE_OPTIONS
    "-I${IMGUI_PATH}"
    ${SDL_INCLUDE_OPTION}
    -DIMGUI_DEFINE_MATH_OPERATORS
    -DIMGUI_DISABLE_DEMO_WINDOWS
    -DIMGUI_DISABLE_DEBUG_TOOLS
    -DIMGUI_DISABLE_DEFAULT_FONT
)

target_compile_options(ImGui PUBLIC ${IMGUI_COMPILE_OPTIONS})

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

target_include_directories(SkylandersPortalEmulator PRIVATE ../common/ ${STB_IMAGE_PATH})
target_link_libraries(SkylandersPortalEmulator PRIVATE ImGui SDL3::SDL3 ws2_32.lib)
target_compile_options(SkylandersPortalEmulator PRIVATE -Wno-experimental-header-units)
target_compile_definitions(SkylandersPortalEmulator PRIVATE PICO_IPV4_ADDRESS=${PICO_IPV4_ADDRESS})

set(STB_IMAGE_COMPILE_OPTIONS -DSTB_IMAGE_IMPLEMENTATION -DSTB_IMAGE_STATIC)
include(HeaderUnitSupport.cmake)

target_header_units(SkylandersPortalEmulator PRIVATE
    HEADER_PATH "${IMGUI_PATH}/backends/imgui_impl_opengl3.h" COMPILE_OPTIONS IMGUI_COMPILE_OPTIONS
    HEADER_PATH "${IMGUI_PATH}/backends/imgui_impl_opengl3_loader.h" COMPILE_OPTIONS IMGUI_COMPILE_OPTIONS
    HEADER_PATH "${IMGUI_PATH}/backends/imgui_impl_sdl3.h" COMPILE_OPTIONS IMGUI_COMPILE_OPTIONS
    HEADER_PATH "${IMGUI_PATH}/imgui_internal.h" COMPILE_OPTIONS IMGUI_COMPILE_OPTIONS
    HEADER_PATH "${IMGUI_PATH}/imgui.h" COMPILE_OPTIONS IMGUI_COMPILE_OPTIONS
    HEADER_PATH "${SDL_PATH}/include/SDL3/SDL.h" COMPILE_OPTIONS SDL_INCLUDE_OPTION
    HEADER_PATH "${CMAKE_CURRENT_LIST_DIR}/../common/Common.hpp" COMPILE_OPTIONS NONE
    HEADER_PATH "${STB_IMAGE_PATH}/stb_image.h" COMPILE_OPTIONS STB_IMAGE_COMPILE_OPTIONS
    HEADER_PATH "${WINSOCK_HEADER_PATH}/winsock2.h" COMPILE_OPTIONS NONE
    HEADER_PATH "${WINSOCK_HEADER_PATH}/ws2tcpip.h" COMPILE_OPTIONS NONE
)