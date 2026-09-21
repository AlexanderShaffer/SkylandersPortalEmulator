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
#include <condition_variable>
#include <filesystem>
#include <SDL3/SDL.h>
#include <imgui.h>
#include <queue>
#include <stop_token>
#include <simpleble/SimpleBLE.h>
#include <thread>
export module PortalEmulator;

import PortalSlot;
import Owner;
import Gate;

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
    bool requestUnload(const std::shared_ptr<PortalSlot>& portalSlot);
    [[nodiscard]] ConnectionStatus getConnectionStatus();

private:

    void scanForPico(const std::stop_token& token);
    void validatePico();
    void disconnectFromPico(const std::stop_token& token, std::string_view message);
    void unloadAllPortalSlots();
    void connectToPico(const std::stop_token& token);
    void runBluetoothThread(const std::stop_token& token);
    [[nodiscard]] bool requestPlayableHalfLoad(const std::shared_ptr<PortalSlot>& portalSlot, PacketType packetType, const std::stop_token& token);
    [[nodiscard]] bool requestPlayableLoad(const std::shared_ptr<PortalSlot>& portalSlot, const std::stop_token& token);
    [[nodiscard]] bool requestPlayableHalfUnload(std::shared_ptr<PortalSlot>& portalSlot, PacketType packetType, const std::stop_token& /* token */);
    [[nodiscard]] bool writeRequest(std::span<uint8_t> packet);
    void writeCommand(std::int32_t data);
    void popWriteRequest();
    void respondToIndication(const SimpleBLE::ByteArray&);
    void runQueuedAudioBytesSender(const std::stop_token& token);
    void respondToMusicNotification(const SimpleBLE::ByteArray&);
    bool validatePortalSlots(const std::stop_token& token);
    void runWriteRequester(const std::stop_token& token);
    void setConnectionStatus(const ConnectionStatus& connectionStatus);

private:

    std::array<std::shared_ptr<PortalSlot>, PORTAL_SLOT_COUNT> m_portalSlots{};
    std::array<std::uint8_t, FIGURE_DUMP_SIZE> m_unloadingFigureDump;
    Gate m_disconnectionGate{};
    Gate m_indicationGate{};
    Gate m_musicGate{};
    std::condition_variable_any m_writeCondition{};
    std::mutex m_mutex{};
    std::queue<std::function<bool(const std::stop_token&)>> m_writeRequests{};
    ConnectionStatus m_connectionStatus{};
    SimpleBLE::Peripheral m_pico{};
    SimpleBLE::Service m_service{};
    SimpleBLE::Characteristic m_requestCharacteristic{};
    SimpleBLE::Characteristic m_musicCharacteristic{};
    std::atomic<std::chrono::time_point<std::chrono::system_clock>> m_timeDuringLastMusicReceived{};
    SDL_AudioStream* m_audioStream{};

    std::jthread m_bluetoothThread{std::bind_front(&PortalEmulator::runBluetoothThread, this)};
};