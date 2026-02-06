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

# CMake 4.2.3 does not support C++ header units. The functions below artificially provide header unit support. However,
# they are only compatible with the Clang C++ compiler. If a header file is modified, its corresponding header unit will
# automatically be recompiled. Finally, the first CMake build will generate the build files correctly, yet display an
# error related to header units. This error is expected and should be ignored. Any subsequent builds should display no errors.

function(target_header_unit TARGET SCOPE HEADER_PATH)
    cmake_path(GET HEADER_PATH STEM HEADER_STEM)
    set(OUTPUT_PATH "${CMAKE_BINARY_DIR}/header_units/${HEADER_STEM}.pcm")

    set(ONE_VALUE_KEYWORDS INCLUDE_PATH)
    cmake_parse_arguments(PARSE_ARGV 0 ARGS "" "${ONE_VALUE_KEYWORDS}" "")

    if(ARGS_INCLUDE_PATH)
        set(INCLUDE_FLAG -I ${ARGS_INCLUDE_PATH})
    endif()

    add_custom_command(
        OUTPUT ${OUTPUT_PATH}
        COMMAND clang++ -std=c++23 -fmodule-header ${HEADER_PATH} ${INCLUDE_FLAG} -o ${OUTPUT_PATH}
        DEPENDS ${HEADER_PATH}
        VERBATIM
        COMMENT "Precompiled header unit from ${HEADER_PATH}"
    )

    target_sources(${TARGET} ${SCOPE} ${OUTPUT_PATH})
    target_compile_options(${TARGET} ${SCOPE} -fmodule-file=${OUTPUT_PATH})
endfunction()

function(target_header_units TARGET SCOPE)
    set(MULTI_VALUE_KEYWORDS ABSOLUTE_PATH INCLUDE_PATH RELATIVE_PATH)
    cmake_parse_arguments(PARSE_ARGV 0 ARGS "" "" "${MULTI_VALUE_KEYWORDS}")

    foreach(ARG ${ARGS_ABSOLUTE_PATH})
        target_header_unit(${TARGET} ${SCOPE} ${ARG})
    endforeach()

    foreach(ARG IN ZIP_LISTS ARGS_INCLUDE_PATH ARGS_RELATIVE_PATH)
        target_header_unit(${TARGET} ${SCOPE} "${ARG_0}/${ARG_1}" INCLUDE_PATH ${ARG_0})
    endforeach()
endfunction()