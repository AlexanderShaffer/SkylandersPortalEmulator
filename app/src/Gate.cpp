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

module;
#include <mutex>
#include <stop_token>
module Gate;

void Gate::openIfNotAlready()
{
    {
        std::lock_guard lock{m_mutex};
        m_open = true;
    }

    m_conditionVariable.notify_all();
}

void Gate::closeIfNotAlready()
{
    std::lock_guard lock{m_mutex};
    m_open = false;
}

void Gate::enterThroughAsSoonAsPossible(const std::stop_token& token)
{
    std::unique_lock lock{m_mutex};
    m_conditionVariable.wait(lock, token, [this] {return m_open;});
}

[[nodiscard]] bool Gate::isOpen()
{
    std::lock_guard lock{m_mutex};
    return m_open;
}