/*
 * This file is part of Skylanders Portal Emulator.
 * Copyright (C) 2026  Alexander Shaffer <alexander.shaffer.623@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include <inttypes.h>

enum PacketType : char
{
    VALIDATE_PORTAL_SLOTS,
    LOAD_FIGURE_HALF_1,
    LOAD_FIGURE_HALF_2,
    UNLOAD_FIGURE_HALF_1,
    UNLOAD_FIGURE_HALF_2
};

enum class PortalSlotState : char
{
    LOADING,
    LOADED,
    UNLOADING,
    UNLOADED
};

constexpr inline int PORTAL_SLOT_COUNT{16};
constexpr inline int FIGURE_DUMP_SIZE{1024};
constexpr inline int FIGURE_HALF_DUMP_SIZE{FIGURE_DUMP_SIZE / 2};
constexpr inline int MAX_PACKET_SIZE{sizeof(PacketType) + sizeof(uint8_t) + FIGURE_HALF_DUMP_SIZE};