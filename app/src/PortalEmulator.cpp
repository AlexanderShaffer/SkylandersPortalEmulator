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
#include <simpleble/SimpleBLE.h>
#include <Common.hpp>
#include <SDL3/SDL.h>
module PortalEmulator;

PortalEmulator::PortalEmulator()
{
    static constexpr SDL_AudioSpec AUDIO_SPEC{.format = SDL_AUDIO_S16LE, .channels = 1, .freq = 8000};

    m_audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &AUDIO_SPEC, nullptr, nullptr);

    if (m_audioStream)
        SDL_ResumeAudioStreamDevice(m_audioStream);
}

PortalEmulator::~PortalEmulator()
{
    SDL_DestroyAudioStream(m_audioStream);
}

[[nodiscard]] std::shared_ptr<PortalSlot> PortalEmulator::linkPortalSlot(const std::filesystem::path& figureDumpPath)
{
    PortalSlotIndex firstAvailable{0};
    std::unique_lock lock{m_mutex};

    while (m_portalSlots[firstAvailable])
        if (++firstAvailable >= m_portalSlots.size())
            return nullptr;

    const auto& portalSlot{m_portalSlots[firstAvailable] = std::make_shared<PortalSlot>(firstAvailable, figureDumpPath)};

    portalSlot->setState(PortalSlotState::LOADING);
    m_writeRequests.emplace(std::bind_front(&PortalEmulator::requestPlayableLoad, this, std::cref(portalSlot)));
    lock.unlock();
    m_writeCondition.notify_all();
    return portalSlot;
}

bool PortalEmulator::requestUnload(const std::shared_ptr<PortalSlot>& portalSlot)
{
    {
        std::lock_guard lock{m_mutex};

        if (portalSlot->getState() != PortalSlotState::LOADED)
            return false;

        auto ref{std::ref(m_portalSlots[portalSlot->getIndex()])};

        portalSlot->setState(PortalSlotState::UNLOADING);
        m_writeRequests.emplace(std::bind_front(&PortalEmulator::requestPlayableUnload, this, ref));
    }

    m_writeCondition.notify_all();
    return true;
}

[[nodiscard]] auto PortalEmulator::getConnectionStatus() -> ConnectionStatus
{
    std::lock_guard lock{m_mutex};
    return m_connectionStatus;
}

void PortalEmulator::scanForPico(const std::stop_token& token)
{
    if (!SimpleBLE::Adapter::bluetooth_enabled())
    {
        setConnectionStatus({.Message = "Bluetooth is unavailable or permission has not been granted", .Color = {1.0f, 0.0f, 0.0f, 1.0f}});
        return;
    }

    std::vector adapters{SimpleBLE::Adapter::get_adapters()};

    if (adapters.empty())
    {
        setConnectionStatus({.Message = "No Bluetooth adapters detected", .Color = {1.0f, 0.5f, 0.0f, 1.0f}});
        return;
    }

    SimpleBLE::Adapter& adapter{adapters.front()};
    std::mutex mutex{};
    std::condition_variable_any stopCondition{};

    adapter.set_callback_on_scan_found([&](SimpleBLE::Peripheral peripheral)
    {
        if (!m_pico.initialized() && peripheral.is_connectable() && peripheral.identifier() == "sky_portal_emulator")
        {
            {
                std::lock_guard lock{mutex};
                m_pico = peripheral;
            }

            stopCondition.notify_all();
        }
    });

    adapter.scan_start();
    setConnectionStatus({.Message = "Scanning...", .Color = {1.0f, 1.0f, 0.0f, 1.0f}});

    {
        std::unique_lock lock{mutex};

        while (!token.stop_requested() && SimpleBLE::Adapter::bluetooth_enabled() && !m_pico.initialized())
        {
            constexpr auto WAIT_INTERVAL{std::chrono::milliseconds(1000)};
            stopCondition.wait_for(lock, token, WAIT_INTERVAL, []{return false;});
        }
    }

    adapter.scan_stop();
}

void PortalEmulator::validatePico()
{
    for (SimpleBLE::Service& service : m_pico.services())
    {
        if (service.uuid() != "facef73e-132f-4415-bd65-73a2123d70a2")
            continue;

        const std::vector characteristics{service.characteristics()};

        if (characteristics.size() != 1)
            return;

        if (SimpleBLE::Characteristic characteristic{characteristics.front()}; characteristic.uuid() == "022dcf96-388f-4531-99ef-1cf30449ac93" &&
            characteristic.can_write_request() && characteristic.can_indicate() && characteristic.can_notify() && !characteristic.can_read() && !characteristic.can_write_command())
        {
            m_service = service;
            m_characteristic = characteristic;
        }
    }
}

void PortalEmulator::disconnectFromPico()
{
    try
    {
        if (m_pico.initialized())
            m_pico.disconnect();
    }
    catch (const SimpleBLE::Exception::BaseException& e)
    {
        std::println(std::cerr, "A SimpleBLE exception occurred while disconnecting from the pico: {}", e.what());
    }

    setConnectionStatus({.Message = "Disconnected", .Color = {0.1f, 1.0f, 0.0f, 1.0f}});
    m_pico = {};
    m_service = {};
    m_characteristic = {};
}

void PortalEmulator::connectToPico(const std::stop_token& token)
{
    m_disconnectionGate.closeIfNotAlready();
    m_pico.set_callback_on_disconnected([this]{m_disconnectionGate.openIfNotAlready();});
    setConnectionStatus({.Message = "Connecting...", .Color = {0.75f, 1.0f, 0.0f, 1.0f}});
    m_pico.connect();
    validatePico();

    if (!m_service.initialized() || !m_characteristic.initialized())
        return;

    m_pico.indicate(m_service.uuid(), m_characteristic.uuid(), std::bind_front(&PortalEmulator::respondToIndication, this));
    m_pico.notify(m_service.uuid(), m_characteristic.uuid(), std::bind_front(&PortalEmulator::respondToNotification, this));
    std::jthread writeRequester{std::bind_front(&PortalEmulator::runWriteRequester, this)};

    setConnectionStatus({.Message = "Connected", .Color = {0.0f, 1.0f, 0.0f, 1.0f}, .Connected = true});
    m_disconnectionGate.enterThroughAsSoonAsPossible(token);
}

void PortalEmulator::runBluetoothThread(const std::stop_token& token)
{
    while (!token.stop_requested())
    {
        try
        {
            disconnectFromPico();
            scanForPico(token);

            if (!m_pico.initialized())
            {
                constexpr auto WAIT_INTERVAL{std::chrono::milliseconds(1000)};
                std::condition_variable_any stopCondition{};
                std::mutex mutex{};
                std::unique_lock lock{mutex};
                stopCondition.wait_for(lock, token, WAIT_INTERVAL, []{return false;});
                continue;
            }

            connectToPico(token);
        }
        catch (const SimpleBLE::Exception::BaseException& e)
        {
            std::println(std::cerr, "A SimpleBLE exception occurred in the Bluetooth thread: {}", e.what());
        }
    }

    disconnectFromPico();
}

bool PortalEmulator::requestPlayableLoad(const std::shared_ptr<PortalSlot>& portalSlot)
{
    std::array<uint8_t, sizeof(PacketType) + sizeof(uint8_t) + FIGURE_DUMP_SIZE> packet;
    auto* pos = packet.data();

    *pos++ = PacketType::LOAD_FIGURE;
    *pos++ = static_cast<uint8_t>(portalSlot->getIndex());
    portalSlot->readSkylanderDump({pos, FIGURE_DUMP_SIZE});

    return writeRequest(packet);
}

bool PortalEmulator::requestPlayableUnload(std::shared_ptr<PortalSlot>& portalSlot)
{
    if (portalSlot->getState() == PortalSlotState::UNLOADED)
    {
        portalSlot = nullptr;
        popWriteRequest();
        return true;
    }

    std::array<uint8_t, sizeof(PacketType) + sizeof(uint8_t)> packet{PacketType::UNLOAD_FIGURE, static_cast<uint8_t>(portalSlot->getIndex())};
    return writeRequest(packet);
}

bool PortalEmulator::writeRequest(const std::span<uint8_t> packet)
{
    try
    {
        m_pico.write_request(m_service.uuid(), m_characteristic.uuid(), {packet.data(), packet.size()});
        return true;
    }
    catch (const SimpleBLE::Exception::BaseException& e)
    {
        std::println(std::cerr, "A SimpleBLE exception occurred while performing a write request: {}", e.what());
        return false;
    }
}

void PortalEmulator::popWriteRequest()
{
    m_writeRequests.pop();
    m_indicationGate.openIfNotAlready();
}

void PortalEmulator::respondToIndication(const SimpleBLE::ByteArray& packet)
{
    std::lock_guard lock{m_mutex};

    if (packet[0] == PacketType::VALIDATE_PORTAL_SLOTS)
    {
        for (int i = 0; i < m_portalSlots.size(); i++)
        {
            const auto& portalSlot{m_portalSlots[i]};
            const auto appSlotState{portalSlot ? portalSlot->getState() : PortalSlotState::UNLOADED};
            const auto picoSlotLoaded{static_cast<bool>(packet[1 + i])};

            if (appSlotState == PortalSlotState::LOADED && !picoSlotLoaded)
            {
                portalSlot->setState(PortalSlotState::LOADING);
                m_writeRequests.emplace(std::bind_front(&PortalEmulator::requestPlayableLoad, this, std::cref(portalSlot)));
            }
            else if (appSlotState == PortalSlotState::UNLOADING && !picoSlotLoaded)
                portalSlot->setState(PortalSlotState::UNLOADED);
        }
    }
    else if (packet[0] == PacketType::LOAD_FIGURE)
    {
        const auto portalSlotIndex{packet[1]};
        const auto& portalSlot{m_portalSlots[portalSlotIndex]};

        if(portalSlot->getState() != PortalSlotState::LOADING)
            std::println(std::cerr, "The portal slot should be in the LOADING state before transitioning to the LOADED state");

        portalSlot->setState(PortalSlotState::LOADED);
    }
    else if (packet[0] == PacketType::UNLOAD_FIGURE)
    {
        const auto portalSlotIndex{packet[1]};
        auto& portalSlot{m_portalSlots[portalSlotIndex]};

        if(portalSlot->getState() != PortalSlotState::UNLOADING)
            std::println(std::cerr, "The portal slot should be in the UNLOADING state before transitioning to the UNLOADED state");

        portalSlot->writeSkylanderDump(packet.slice_from(2));
        portalSlot->setState(PortalSlotState::UNLOADED);
        portalSlot = nullptr;
    }
    else
    {
        std::println(std::cerr, "Unknown packet type received: {}", packet[0]);
        return;
    }

    popWriteRequest();
}

void PortalEmulator::respondToNotification(const SimpleBLE::ByteArray& packet) const
{
    if (!m_audioStream)
        return;

    if (!SDL_PutAudioStreamData(m_audioStream, packet.data(), static_cast<int>(packet.size())))
        std::println(std::cerr, "Failed to put audio stream data: {}", SDL_GetError());
}

bool PortalEmulator::validatePortalSlots()
{
    std::lock_guard lock{m_mutex};
    std::array<uint8_t, sizeof(PacketType) + PORTAL_SLOT_COUNT> packet;

    packet[0] = PacketType::VALIDATE_PORTAL_SLOTS;

    for (int i = 0; i < m_portalSlots.size(); i++)
        packet[1 + i] = static_cast<uint8_t>(m_portalSlots[i] ? m_portalSlots[i]->getState() : PortalSlotState::UNLOADED);

    return writeRequest(packet);
}

void PortalEmulator::runWriteRequester(const std::stop_token& token)
{
    if (!validatePortalSlots())
    {
        m_disconnectionGate.openIfNotAlready();
        return;
    }

    while (!token.stop_requested())
    {
        std::unique_lock lock{m_mutex};
        m_writeCondition.wait(lock, token, [this]{return !m_writeRequests.empty();});

        if (token.stop_requested())
            break;

        const auto& request{m_writeRequests.front()};

        m_indicationGate.closeIfNotAlready();

        if (!request())
            break;

        lock.unlock();
        m_indicationGate.enterThroughAsSoonAsPossible(token);
    }

    m_disconnectionGate.openIfNotAlready();
}

void PortalEmulator::setConnectionStatus(const ConnectionStatus& connectionStatus)
{
    std::lock_guard lock{m_mutex};
    m_connectionStatus = connectionStatus;
}