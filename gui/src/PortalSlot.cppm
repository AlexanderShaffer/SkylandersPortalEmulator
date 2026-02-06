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

export module PortalSlot;

import "Common.hpp";
import std;

export class PortalSlot
{
public:

    explicit PortalSlot(int index, const std::filesystem::path& figureDumpPath);

    void setState(PortalSlotState state) {m_state = state;}
    void readSkylanderDump(std::span<uint8_t> output);
    void writeSkylanderDump(std::span<uint8_t> input);
    [[nodiscard]] int getIndex() const {return m_index;}
    [[nodiscard]] PortalSlotState getState() const {return m_state;}

private:

    int m_index{};
    std::fstream m_skylanderDump{};
    PortalSlotState m_state{PortalSlotState::LOADING};
};