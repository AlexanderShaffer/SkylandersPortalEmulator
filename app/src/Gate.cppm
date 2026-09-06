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

export module Gate;

import std;

export class Gate
{
public:

    Gate() = default;
    Gate(const Gate& other) = delete;
    Gate(Gate&& other) = delete;
    void operator=(const Gate& other) = delete;
    void operator=(Gate&& other) = delete;

    void openIfNotAlready();
    void closeIfNotAlready();
    void enterThroughAsSoonAsPossible(const std::stop_token& token);
    [[nodiscard]] bool isOpen();

private:

    bool m_open{false};
    std::mutex m_mutex{};
    std::condition_variable_any m_conditionVariable{};
};