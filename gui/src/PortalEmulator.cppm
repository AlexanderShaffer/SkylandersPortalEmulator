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

export module PortalEmulator;

import "Common.hpp";
import <SDL3/SDL.h>;
import <winsock2.h>;
import std;
import PortalSlot;
import Owner;

export class PortalEmulator
{
public:

    using PortalSlotIndex = int;

public:

    PortalEmulator() = default;
    ~PortalEmulator();
    PortalEmulator(const PortalEmulator&) = delete;
    PortalEmulator(PortalEmulator&&) = delete;
    PortalEmulator& operator=(const PortalEmulator&) = delete;
    PortalEmulator& operator=(PortalEmulator&&) = delete;

    [[nodiscard]] std::shared_ptr<PortalSlot> linkPortalSlot(const std::filesystem::path& figureDumpPath);
    [[nodiscard]] bool requestUnload(const std::shared_ptr<PortalSlot>& portalSlot);
    [[nodiscard]] bool isConnected() const {return m_connected;}

private:

    [[nodiscard]] bool connectToTcpClient();
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

private:

    std::array<std::shared_ptr<PortalSlot>, PORTAL_SLOT_COUNT> m_portalSlots{};
    std::condition_variable m_sendCondition{};
    std::condition_variable m_receiveCondition{};
    std::condition_variable m_disconnectionCondition{};
    std::mutex m_mutex{};
    bool m_receivedTcpMessage{};
    std::deque<std::function<bool()>> m_sendRequests{};
    bool m_connected{};
    SOCKET m_tcpClient{INVALID_SOCKET};
    SOCKET m_udpClient{INVALID_SOCKET};
    sockaddr m_clientAddress{};

    //TODO: Consider when multiple Raspberry Pis can connect to the same computer
    std::jthread m_networkThread{std::bind_front(&PortalEmulator::runNetworkThread, this)};
};