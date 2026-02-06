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

module PortalSlot;

PortalSlot::PortalSlot(const int index, const std::filesystem::path& figureDumpPath) : m_index{index}, m_skylanderDump{figureDumpPath, std::ios::binary | std::ios::in | std::ios::out}
{
    m_skylanderDump.exceptions(std::ios::failbit | std::ios::badbit);
}

void PortalSlot::readSkylanderDump(const std::span<uint8_t> output)
{
    m_skylanderDump.seekg(std::ios::beg);
    m_skylanderDump.read(reinterpret_cast<char*>(output.data()), output.size());
}

void PortalSlot::writeSkylanderDump(const std::span<uint8_t> input)
{
    m_skylanderDump.seekg(std::ios::beg);
    m_skylanderDump.write(reinterpret_cast<const char*>(input.data()), input.size());
}