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
#include <Common.hpp>
#include <SDL3/SDL.h>
#include <imgui.h>
#include <simpleble/SimpleBLE.h>
export module PortalEmulator;

import PortalSlot;
import Owner;
import std;

export struct ConnectionStatus
{
    std::string_view Message{};
    ImVec4 Color{};
};

export class PortalEmulator
{
public:

    using PortalSlotIndex = int;

public:

    PortalEmulator() = default;
    PortalEmulator(const PortalEmulator&) = delete;
    PortalEmulator(PortalEmulator&&) = delete;
    PortalEmulator& operator=(const PortalEmulator&) = delete;
    PortalEmulator& operator=(PortalEmulator&&) = delete;

    [[nodiscard]] std::shared_ptr<PortalSlot> linkPortalSlot(const std::filesystem::path& figureDumpPath);
    [[nodiscard]] bool requestUnload(const std::shared_ptr<PortalSlot>& portalSlot);
    [[nodiscard]] bool isConnected() const {return m_connected;}
    [[nodiscard]] ConnectionStatus getConnectionStatus();

private:

    [[nodiscard]] bool connectToTcpClient();
    SimpleBLE::Peripheral connectToPico(const std::stop_token& token);
    [[nodiscard]] bool validatePortalSlots() const;
    void runNetworkThread(const std::stop_token& token);
    void runTcpReceiver(const std::stop_token& token);
    [[nodiscard]] bool requestPlayableLoad(const std::shared_ptr<PortalSlot>& portalSlot) const;
    [[nodiscard]] bool requestPlayableUnload(std::shared_ptr<PortalSlot>& portalSlot);
    [[nodiscard]] bool tcpSend(std::span<uint8_t> packet) const;
    [[nodiscard]] bool tcpReceive(std::span<uint8_t> buffer) const;
    void respond(std::span<uint8_t> packet);
    void runTcpSender(const std::stop_token& token);
    void runUdpReceiver(const std::stop_token& token);
    void setConnectionStatus(const ConnectionStatus& connectionStatus);

private:

    std::array<std::shared_ptr<PortalSlot>, PORTAL_SLOT_COUNT> m_portalSlots{};
    std::condition_variable m_sendCondition{};
    std::condition_variable m_receiveCondition{};
    std::condition_variable m_disconnectionCondition{};
    std::mutex m_statusMutex{};
    std::mutex m_connectionMutex{};
    bool m_receivedTcpMessage{};
    std::deque<std::function<bool()>> m_sendRequests{};
    bool m_connected{};
    ConnectionStatus m_connectionStatus{};

    std::jthread m_networkThread{std::bind_front(&PortalEmulator::runNetworkThread, this)};
};