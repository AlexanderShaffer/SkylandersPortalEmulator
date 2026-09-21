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
#include <filesystem>
#include <mutex>
#include <thread>
#include <condition_variable>
module PortalEmulator;

PortalEmulator::PortalEmulator()
{
    static constexpr SDL_AudioSpec AUDIO_SPEC{.format = SDL_AUDIO_S16LE, .channels = 1, .freq = 8000};
    m_audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &AUDIO_SPEC, nullptr, nullptr);

    if (m_audioStream)
        SDL_ResumeAudioStreamDevice(m_audioStream);
    else
        std::println(std::cerr, "Failed to initialize the audio device stream: {}", SDL_GetError());
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
        m_writeRequests.emplace(std::bind_front(&PortalEmulator::requestPlayableHalfUnload, this, ref, PacketType::UNLOAD_FIGURE_HALF_1));
        m_writeRequests.emplace(std::bind_front(&PortalEmulator::requestPlayableHalfUnload, this, ref, PacketType::UNLOAD_FIGURE_HALF_2));
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

namespace
{
template<typename Item>
Item* get(std::vector<Item>& items, const std::string_view uuid)
{
    for (Item& item : items)
        if (item.uuid() == uuid)
            return &item;

    return nullptr;
}
} // namespace

void PortalEmulator::validatePico()
{
    static constexpr std::size_t EXPECTED_SERVICE_COUNT{3};
    std::vector services{m_pico.services()};

    if (services.size() != EXPECTED_SERVICE_COUNT)
        return;

    SimpleBLE::Service* service{get(services, "facef73e-132f-4415-bd65-73a2123d70a2")};

    if (!service)
        return;

    static constexpr std::size_t EXPECTED_CHARACTERISTIC_COUNT{2};
    std::vector characteristics{service->characteristics()};

    if (characteristics.size() != EXPECTED_CHARACTERISTIC_COUNT)
        return;

    SimpleBLE::Characteristic* request{get(characteristics, "022dcf96-388f-4531-99ef-1cf30449ac93")};

    if (!request || !request->can_write_request() || !request->can_indicate() || request->can_write_command() || request->can_read() || request->can_notify())
        return;

    SimpleBLE::Characteristic* music{get(characteristics, "d77f953a-932d-40d4-b4cd-7310bea9c201")};

    if (!music || !music->can_write_command() || !music->can_notify() || music->can_indicate() || music->can_write_request() || music->can_read())
        return;

    m_service = *service;
    m_requestCharacteristic = *request;
    m_musicCharacteristic = *music;
}

namespace
{
void sleep(const std::stop_token& token, const std::chrono::milliseconds duration)
{
    std::condition_variable_any stopCondition{};
    std::mutex mutex{};
    std::unique_lock lock{mutex};
    stopCondition.wait_for(lock, token, duration, []{return false;});
}
} // namespace

void PortalEmulator::disconnectFromPico(const std::stop_token& token, const std::string_view message)
{
    try
    {
        if (m_pico.initialized())
            m_pico.disconnect();
    }
    catch (const std::exception& /* e */) {}

    if (m_pico.initialized())
    {
        setConnectionStatus({.Message = message, .Color = {1.0f, 0.5f, 0.0f, 1.0f}});
        sleep(token, std::chrono::milliseconds{5000});
    }

    m_pico = {};
    m_service = {};
    m_requestCharacteristic = {};
    m_musicCharacteristic = {};
}

void PortalEmulator::unloadAllPortalSlots()
{
    for (std::shared_ptr<PortalSlot>& portalSlot : m_portalSlots)
        if (portalSlot)
            requestUnload(portalSlot);

    std::unique_lock lock{m_mutex};
    m_writeCondition.wait(lock, [this]{return m_writeRequests.empty() || m_disconnectionGate.isOpen();});
}

void PortalEmulator::connectToPico(const std::stop_token& token)
{
    m_disconnectionGate.closeIfNotAlready();
    m_pico.set_callback_on_disconnected([this]{m_disconnectionGate.openIfNotAlready(); m_writeCondition.notify_all();});
    setConnectionStatus({.Message = "Connecting...", .Color = {0.75f, 1.0f, 0.0f, 1.0f}});
    m_pico.connect();
    validatePico();

    if (!m_service.initialized() || !m_requestCharacteristic.initialized() || !m_musicCharacteristic.initialized())
    {
        disconnectFromPico(token, "Disconnected - The Pico W's software version is incompatible");
        return;
    }

    m_pico.indicate(m_service.uuid(), m_requestCharacteristic.uuid(), std::bind_front(&PortalEmulator::respondToIndication, this));
    m_pico.notify(m_service.uuid(), m_musicCharacteristic.uuid(), std::bind_front(&PortalEmulator::respondToMusicNotification, this));
    std::jthread writeRequester{std::bind_front(&PortalEmulator::runWriteRequester, this)};
    std::jthread queuedAudioBytesSender{std::bind_front(&PortalEmulator::runQueuedAudioBytesSender, this)};

    m_disconnectionGate.enterThroughAsSoonAsPossible(token);
    m_pico.unsubscribe(m_service.uuid(), m_musicCharacteristic.uuid());

    if (token.stop_requested())
        unloadAllPortalSlots();

    m_pico.unsubscribe(m_service.uuid(), m_requestCharacteristic.uuid());
}

void PortalEmulator::runBluetoothThread(const std::stop_token& token)
{
    while (!token.stop_requested())
    {
        try
        {
            disconnectFromPico(token, "Disconnected - Ensure the Pico W has a clear line of sight to this device");
            scanForPico(token);

            if (!m_pico.initialized())
            {
                sleep(token, std::chrono::milliseconds{1000});
                continue;
            }

            connectToPico(token);
        }
        catch (const std::exception& /* e */) {}
    }

    disconnectFromPico(token, "Disconnected - Stop requested");
}

bool PortalEmulator::requestPlayableHalfLoad(const std::shared_ptr<PortalSlot>& portalSlot, const PacketType packetType, const std::stop_token& token)
{
    if (token.stop_requested())
        return true;

    std::array<uint8_t, sizeof(PacketType) + sizeof(uint8_t) + FIGURE_HALF_DUMP_SIZE> packet;
    auto* pos = packet.data();

    *pos++ = packetType;

    {
        std::lock_guard lock{m_mutex};

        *pos++ = static_cast<uint8_t>(portalSlot->getIndex());
        portalSlot->readSkylanderDump(packetType == PacketType::LOAD_FIGURE_HALF_1 ? 0 : FIGURE_HALF_DUMP_SIZE, {pos, FIGURE_HALF_DUMP_SIZE});
    }

    m_indicationGate.closeIfNotAlready();

    if (!writeRequest(packet))
        return false;

    m_indicationGate.enterThroughAsSoonAsPossible(token);
    return true;
}

bool PortalEmulator::requestPlayableLoad(const std::shared_ptr<PortalSlot>& portalSlot, const std::stop_token& token)
{
    return requestPlayableHalfLoad(portalSlot, PacketType::LOAD_FIGURE_HALF_1, token) && requestPlayableHalfLoad(portalSlot, PacketType::LOAD_FIGURE_HALF_2, token);
}

bool PortalEmulator::requestPlayableHalfUnload(std::shared_ptr<PortalSlot>& portalSlot, const PacketType packetType, const std::stop_token& /* token */)
{
    std::unique_lock lock{m_mutex};

    if (!portalSlot)
    {
        popWriteRequest();
        return true;
    }

    std::array packet{static_cast<uint8_t>(packetType), static_cast<uint8_t>(portalSlot->getIndex())};
    lock.unlock();
    return writeRequest(packet);
}

bool PortalEmulator::writeRequest(const std::span<uint8_t> packet)
{
    try
    {
        m_pico.write_request(m_service.uuid(), m_requestCharacteristic.uuid(), {packet.data(), packet.size()});
        return true;
    }
    catch (const std::exception& /* e */)
    {
        return false;
    }
}

void PortalEmulator::writeCommand(const std::int32_t data)
{
    try
    {
        m_pico.write_command(m_service.uuid(), m_musicCharacteristic.uuid(), {reinterpret_cast<const std::uint8_t*>(&data), sizeof(data)});
    }
    catch (const std::exception& /* e */) {}
}

void PortalEmulator::popWriteRequest()
{
    m_writeRequests.pop();
    m_indicationGate.openIfNotAlready();
}

void PortalEmulator::respondToIndication(const SimpleBLE::ByteArray& packet)
{
    std::lock_guard lock{m_mutex};
    const std::uint8_t type{packet[0]};

    if (type == PacketType::VALIDATE_PORTAL_SLOTS)
    {
        for (int i = 0; i < m_portalSlots.size(); i++)
        {
            auto& portalSlot{m_portalSlots[i]};
            const auto appSlotState{portalSlot ? portalSlot->getState() : PortalSlotState::UNLOADED};
            const auto picoSlotLoaded{static_cast<bool>(packet[1 + i])};

            if (appSlotState == PortalSlotState::LOADED && !picoSlotLoaded)
            {
                portalSlot->setState(PortalSlotState::LOADING);
                m_writeRequests.emplace(std::bind_front(&PortalEmulator::requestPlayableLoad, this, std::cref(portalSlot)));
            }
            else if (appSlotState == PortalSlotState::UNLOADING && !picoSlotLoaded)
            {
                portalSlot->setState(PortalSlotState::UNLOADED);
                portalSlot = nullptr;
            }
        }

        m_indicationGate.openIfNotAlready();
        return;
    }

    if (type == PacketType::LOAD_FIGURE_HALF_2)
    {
        const auto portalSlotIndex{packet[1]};
        const auto& portalSlot{m_portalSlots[portalSlotIndex]};

        if(portalSlot->getState() != PortalSlotState::LOADING)
            std::println(std::cerr, "The portal slot should be in the LOADING state before transitioning to the LOADED state");

        portalSlot->setState(PortalSlotState::LOADED);
    }
    else if (type == PacketType::UNLOAD_FIGURE_HALF_1 || type == PacketType::UNLOAD_FIGURE_HALF_2)
    {
        const auto portalSlotIndex{packet[1]};
        auto& portalSlot{m_portalSlots[portalSlotIndex]};

        if (type == UNLOAD_FIGURE_HALF_1)
            std::copy_n(packet.data() + 2, FIGURE_HALF_DUMP_SIZE, m_unloadingFigureDump.begin());
        else
        {
            std::copy_n(packet.data() + 2, FIGURE_HALF_DUMP_SIZE, m_unloadingFigureDump.begin() + FIGURE_HALF_DUMP_SIZE);

            if(portalSlot->getState() != PortalSlotState::UNLOADING)
                std::println(std::cerr, "The portal slot should be in the UNLOADING state before transitioning to the UNLOADED state");

            portalSlot->writeSkylanderDump(m_unloadingFigureDump);
            portalSlot->setState(PortalSlotState::UNLOADED);
            portalSlot = nullptr;
        }
    }
    else
    {
        if (type == PacketType::LOAD_FIGURE_HALF_1)
            m_indicationGate.openIfNotAlready();
        else
            std::println(std::cerr, "Unknown packet type received: {}", packet[0]);

        return;
    }

    popWriteRequest();
}

namespace
{
std::int32_t toLittleEndian(const std::int32_t value)
{
    static_assert(std::endian::native == std::endian::little || std::endian::native == std::endian::big);

    if constexpr (std::endian::native == std::endian::little)
        return value;
    else
        return std::byteswap(value);
}
} // namespace

void PortalEmulator::runQueuedAudioBytesSender(const std::stop_token& token)
{
    if (!m_audioStream)
        return;

    while (!token.stop_requested())
    {
        m_musicGate.closeIfNotAlready();
        m_musicGate.enterThroughAsSoonAsPossible(token);

        if (token.stop_requested())
            return;

        static constexpr std::array<std::uint8_t, 1024 * 4> SILENT_AUDIO{};

        SDL_ClearAudioStream(m_audioStream);
        SDL_PutAudioStreamData(m_audioStream, SILENT_AUDIO.data(), SILENT_AUDIO.size());

        static constexpr std::chrono::milliseconds MUSIC_TIMEOUT{250};

        while (!token.stop_requested() && std::chrono::system_clock::now() - m_timeDuringLastMusicReceived.load() < MUSIC_TIMEOUT)
        {
            if (const std::int32_t bytesQueued{SDL_GetAudioStreamQueued(m_audioStream)}; bytesQueued >= 0)
                writeCommand(toLittleEndian(bytesQueued));

            static constexpr std::chrono::milliseconds SEND_INTERVAL{100};
            std::this_thread::sleep_for(SEND_INTERVAL);
        }
    }
}

void PortalEmulator::respondToMusicNotification(const SimpleBLE::ByteArray& packet)
{
    if (!m_audioStream)
        return;

    if (!SDL_PutAudioStreamData(m_audioStream, packet.data(), static_cast<int>(packet.size())))
        std::println(std::cerr, "Failed to put audio stream data: {}", SDL_GetError());

    m_timeDuringLastMusicReceived = std::chrono::system_clock::now();
    m_musicGate.openIfNotAlready();
}

bool PortalEmulator::validatePortalSlots(const std::stop_token& token)
{
    std::array<uint8_t, sizeof(PacketType) + PORTAL_SLOT_COUNT> packet;
    packet[0] = PacketType::VALIDATE_PORTAL_SLOTS;

    {
        std::lock_guard lock{m_mutex};

        for (int i = 0; i < m_portalSlots.size(); i++)
            packet[1 + i] = static_cast<uint8_t>(m_portalSlots[i] ? m_portalSlots[i]->getState() : PortalSlotState::UNLOADED);
    }

    m_indicationGate.closeIfNotAlready();

    if (!writeRequest(packet))
        return false;

    m_indicationGate.enterThroughAsSoonAsPossible(token);
    return true;
}

void PortalEmulator::runWriteRequester(const std::stop_token& token)
{
    if (!validatePortalSlots(token))
    {
        m_disconnectionGate.openIfNotAlready();
        m_writeCondition.notify_all();
        return;
    }

    setConnectionStatus({.Message = "Connected", .Color = {0.0f, 1.0f, 0.0f, 1.0f}, .Connected = true});

    while (!token.stop_requested())
    {
        std::unique_lock lock{m_mutex};
        m_writeCondition.wait(lock, token, [this]{return !m_writeRequests.empty();});

        if (token.stop_requested())
            break;

        const auto& request{m_writeRequests.front()};
        lock.unlock();

        m_indicationGate.closeIfNotAlready();

        if (!request(token))
            break;

        m_indicationGate.enterThroughAsSoonAsPossible(token);
        m_writeCondition.notify_all();
    }

    m_disconnectionGate.openIfNotAlready();
    m_writeCondition.notify_all();
}

void PortalEmulator::setConnectionStatus(const ConnectionStatus& connectionStatus)
{
    std::lock_guard lock{m_mutex};
    m_connectionStatus = connectionStatus;
}