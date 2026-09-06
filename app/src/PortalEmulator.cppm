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
import Gate;
import std;

export class PortalEmulator
{
public:

    using PortalSlotIndex = int;

    struct ConnectionStatus
    {
        std::string_view Message{};
        ImVec4 Color{};
        bool Connected{false};
    };

public:

    PortalEmulator();
    ~PortalEmulator();
    PortalEmulator(const PortalEmulator&) = delete;
    PortalEmulator(PortalEmulator&&) = delete;
    PortalEmulator& operator=(const PortalEmulator&) = delete;
    PortalEmulator& operator=(PortalEmulator&&) = delete;

    [[nodiscard]] std::shared_ptr<PortalSlot> linkPortalSlot(const std::filesystem::path& figureDumpPath);
    [[nodiscard]] bool requestUnload(const std::shared_ptr<PortalSlot>& portalSlot);
    [[nodiscard]] ConnectionStatus getConnectionStatus();

private:

    void scanForPico(const std::stop_token& token);
    void validatePico();
    void disconnectFromPico();
    void connectToPico(const std::stop_token& token);
    void runBluetoothThread(const std::stop_token& token);
    [[nodiscard]] bool requestPlayableLoad(const std::shared_ptr<PortalSlot>& portalSlot);
    [[nodiscard]] bool requestPlayableUnload(std::shared_ptr<PortalSlot>& portalSlot);
    [[nodiscard]] bool writeRequest(std::span<uint8_t> packet);
    void popWriteRequest();
    void respondToIndication(const SimpleBLE::ByteArray&);
    void respondToNotification(const SimpleBLE::ByteArray&) const;
    bool validatePortalSlots();
    void runWriteRequester(const std::stop_token& token);
    void setConnectionStatus(const ConnectionStatus& connectionStatus);

private:

    std::array<std::shared_ptr<PortalSlot>, PORTAL_SLOT_COUNT> m_portalSlots{};
    Gate m_disconnectionGate{};
    Gate m_indicationGate{};
    std::condition_variable_any m_writeCondition{};
    std::mutex m_mutex{};
    std::queue<std::function<bool()>> m_writeRequests{};
    ConnectionStatus m_connectionStatus{};
    SimpleBLE::Peripheral m_pico{};
    SimpleBLE::Service m_service{};
    SimpleBLE::Characteristic m_characteristic{};
    SDL_AudioStream* m_audioStream{};

    std::jthread m_networkThread{std::bind_front(&PortalEmulator::runBluetoothThread, this)};
};