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

# CMake 4.2.3 does not support C++ header units. The functions below artificially provide header unit support. However, they are only
# compatible with the Clang C++ compiler. If a header file is modified, its corresponding header unit will be automatically recompiled.

function(target_header_unit TARGET SCOPE HEADER_PATH COMPILE_OPTIONS)
    cmake_path(GET HEADER_PATH STEM HEADER_STEM)
    set(OUTPUT_PATH "${CMAKE_BINARY_DIR}/header_units/${HEADER_STEM}.pcm")
    set(COMMAND clang++ -std=c++23 ${COMPILE_OPTIONS} -fmodule-header ${HEADER_PATH} -o ${OUTPUT_PATH})
    set(COMMENT "Precompiled header unit from ${HEADER_PATH}")

    if (NOT EXISTS "${OUTPUT_PATH}")
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/header_units")
        execute_process(COMMAND ${COMMAND} COMMAND_ERROR_IS_FATAL ANY)
        message(${COMMENT})
    endif ()

    add_custom_command(OUTPUT ${OUTPUT_PATH} COMMAND ${COMMAND} DEPENDS ${HEADER_PATH} VERBATIM COMMENT ${COMMENT})
    target_sources(${TARGET} ${SCOPE} ${OUTPUT_PATH})
    target_compile_options(${TARGET} ${SCOPE} -fmodule-file=${OUTPUT_PATH})
endfunction()

function(target_header_units TARGET SCOPE)
    set(MULTI_VALUE_ARGS HEADER_PATH COMPILE_OPTIONS)
    cmake_parse_arguments(ARGS "" "" "${MULTI_VALUE_ARGS}" ${ARGN})

    foreach (HEADER_PATH COMPILE_OPTIONS IN ZIP_LISTS ARGS_HEADER_PATH ARGS_COMPILE_OPTIONS)
        if (NOT HEADER_PATH OR NOT COMPILE_OPTIONS)
            message(FATAL_ERROR "Each header unit in target_header_units() must include the header file path and compile options.")
        endif ()

        target_header_unit("${TARGET}" "${SCOPE}" "${HEADER_PATH}" "${${COMPILE_OPTIONS}}")
    endforeach ()
endfunction()