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
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD f35a9ac6-8463-4d38-8eec-5d6008153e7d)

project(SkylandersPortalEmulator)
include(FetchContent)

FetchContent_Declare(ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG 6ded5230d043aa32c755e65c910c2af5002fb9f9 # Release 1.92.6
    GIT_SHALLOW YES
)

FetchContent_Declare(Sdl
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG 683181b47cfabd293e3ea409f838915b8297a4fd # Release 3.4.2
    GIT_SHALLOW YES
)

FetchContent_Declare(Stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 013ac3beddff3dbffafd5177e7972067cd2b5083 # Release 2.30
)

FetchContent_Declare(SimpleBle
    GIT_REPOSITORY https://github.com/simpleble/simpleble.git
    GIT_TAG eee107c5076f0cb57480c47cc9503ea3cf5f5686 # Release 1.1.0
    GIT_SHALLOW YES
)

set(SDL_SHARED OFF)
set(SDL_STATIC ON)
FetchContent_MakeAvailable(ImGui Sdl Stb SimpleBle)
add_subdirectory(${simpleble_SOURCE_DIR}/simpleble ${simpleble_BINARY_DIR})

add_library(ImGui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
)

target_compile_options(ImGui PUBLIC
  -DIMGUI_DEFINE_MATH_OPERATORS
  -DIMGUI_DISABLE_DEMO_WINDOWS
  -DIMGUI_DISABLE_DEBUG_TOOLS
  -DIMGUI_DISABLE_DEFAULT_FONT
)

target_include_directories(ImGui PUBLIC ${imgui_SOURCE_DIR})

set(CMAKE_CXX_MODULE_STD ON)
add_executable(SkylandersPortalEmulator WIN32)

target_sources(SkylandersPortalEmulator
    PRIVATE
    src/Figure.cpp
    src/FigureLoader.cpp
    src/Gate.cpp
    src/Main.cpp
    src/Owner.cpp
    src/PortalEmulator.cpp
    src/PortalSlot.cpp
    src/Texture.cpp

    PRIVATE FILE_SET CXX_MODULES FILES
    src/Figure.cppm
    src/FigureLoader.cppm
    src/Font.cppm
    src/Gate.cppm
    src/Owner.cppm
    src/PortalEmulator.cppm
    src/PortalSlot.cppm
    src/Texture.cppm
)

target_link_libraries(SkylandersPortalEmulator PRIVATE ImGui SDL3::SDL3 simpleble::simpleble)
target_include_directories(SkylandersPortalEmulator PRIVATE ../common/ ${stb_SOURCE_DIR})