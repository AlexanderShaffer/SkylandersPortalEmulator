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

module PortalEmulator;

import <SDL3/SDL.h>;
import <ws2tcpip.h>;

PortalEmulator::~PortalEmulator()
{
    WSACleanup();
}

[[nodiscard]] std::shared_ptr<PortalSlot> PortalEmulator::linkPortalSlot(const std::filesystem::path& figureDumpPath)
{
    PortalSlotIndex firstAvailable{0};

    while (m_portalSlots[firstAvailable])
        if (++firstAvailable >= m_portalSlots.size())
            return nullptr;

    std::unique_lock lock{m_mutex};
    const auto& portalSlot{m_portalSlots[firstAvailable] = std::make_shared<PortalSlot>(firstAvailable, figureDumpPath)};

    m_sendRequests.emplace_back(std::bind_front(&PortalEmulator::requestPlayableLoad, this, std::cref(portalSlot)));
    lock.unlock();
    m_sendCondition.notify_all();
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
        m_sendRequests.emplace_back(std::bind_front(&PortalEmulator::requestPlayableUnload, this, ref));
    }

    m_sendCondition.notify_all();
    return true;
}

bool PortalEmulator::connectToTcpClient()
{
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* address{nullptr};

    if (getaddrinfo(nullptr, std::to_string(TCP_PORT).c_str(), &hints, &address))
        throw std::runtime_error("Failed to get address info");

    SOCKET listenSocket{socket(address->ai_family, address->ai_socktype, address->ai_protocol)};

    if (listenSocket == INVALID_SOCKET)
    {
        freeaddrinfo(address);
        throw std::runtime_error("Failed to create listen socket");
    }

    if (bind(listenSocket, address->ai_addr, address->ai_addrlen) == SOCKET_ERROR)
    {
        freeaddrinfo(address);
        closesocket(listenSocket);
        throw std::runtime_error("Failed to bind listen socket");
    }

    freeaddrinfo(address);

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        closesocket(listenSocket);
        throw std::runtime_error("Failed to listen for connection");
    }

    std::println("Listening for TCP connection");
    int length{sizeof(m_clientAddress)};
    m_tcpClient = accept(listenSocket, &m_clientAddress, &length);

    if (m_tcpClient == INVALID_SOCKET)
    {
        std::println("Failed to accept TCP connection");
        closesocket(listenSocket);
        return false;
    }

    std::println("TCP connection established");
    closesocket(listenSocket);
    return true;
}

bool PortalEmulator::validatePortalSlots() const
{
    std::array<uint8_t, 2 * sizeof(uint8_t) + sizeof(PacketType) + PORTAL_SLOT_COUNT> packet{};

    for (int i = 0; i < m_portalSlots.size(); i++)
        packet[3 + i] = static_cast<uint8_t>(m_portalSlots[i] ? m_portalSlots[i]->getState() : PortalSlotState::UNLOADED);

    packet[2] = PacketType::VALIDATE_PORTAL_SLOTS;
    return tcpSend(packet);
}

void PortalEmulator::runNetworkThread(const std::stop_token& token)
{
    WSADATA wsaData{};

    if (WSAStartup(WINSOCK_VERSION, &wsaData))
        throw std::runtime_error("WSAStartup failed");

    while (!token.stop_requested())
    {
        if (!connectToTcpClient())
            break;

        {
            std::lock_guard lock{m_mutex};
            m_sendRequests.emplace_front(std::bind_front(&PortalEmulator::validatePortalSlots, this));
        }

        m_receivedTcpMessage = false;
        m_connected = true;

        std::jthread tcpReceiver{std::bind_front(&PortalEmulator::runTcpReceiver, this)};
        std::jthread tcpSender{std::bind_front(&PortalEmulator::runTcpSender, this)};
        std::jthread udpReceiver{std::bind_front(&PortalEmulator::runUdpReceiver, this)};

        {
            std::unique_lock lock{m_mutex};
            m_disconnectionCondition.wait(lock, [this]{return !m_connected;});
        }

        udpReceiver.request_stop();
        tcpSender.request_stop();
        tcpReceiver.request_stop();

        shutdown(m_udpClient, SD_BOTH);
        closesocket(m_udpClient);
        shutdown(m_tcpClient, SD_BOTH);
        closesocket(m_tcpClient);
        m_receiveCondition.notify_all();
        m_sendCondition.notify_all();
    }

    std::println("Network thread has terminated");
}

void PortalEmulator::runTcpReceiver(const std::stop_token& token)
{
    std::println("TCP receiver has started");

    std::array<uint8_t, MAX_PACKET_SIZE> buffer;

    while (!token.stop_requested())
    {
        constexpr std::size_t PACKET_SIZE_BYTES{2};

        if (!tcpReceive({buffer.data(), PACKET_SIZE_BYTES}))
            break;

        const std::size_t packetSize{static_cast<std::size_t>(buffer[0] << 8 | buffer[1])};
        const auto packetDataSize{packetSize - PACKET_SIZE_BYTES};
        const std::span packet{buffer.data(), packetDataSize};

        if (!tcpReceive(packet))
            break;

        respond(packet);
    }

    std::println("TCP receiver has terminated");
    m_connected = false;
    m_disconnectionCondition.notify_all();
}

bool PortalEmulator::requestPlayableLoad(const std::shared_ptr<PortalSlot>& portalSlot) const
{
    std::array<uint8_t, 2 * sizeof(uint8_t) + sizeof(PacketType) + sizeof(uint8_t) + FIGURE_DUMP_SIZE> packet;
    auto* pos = packet.data() + 2;

    *pos++ = PacketType::LOAD_FIGURE;
    *pos++ = static_cast<uint8_t>(portalSlot->getIndex());
    portalSlot->readSkylanderDump({pos, FIGURE_DUMP_SIZE});

    return tcpSend(packet);
}

bool PortalEmulator::requestPlayableUnload(std::shared_ptr<PortalSlot>& portalSlot)
{
    if (portalSlot->getState() == PortalSlotState::UNLOADED)
    {
        portalSlot = nullptr;

        {
            std::lock_guard lock{m_mutex};
            m_sendRequests.pop_front();
        }

        m_receivedTcpMessage = true;
        return true;
    }

    std::array<uint8_t, 2 * sizeof(uint8_t) + sizeof(PacketType) + sizeof(uint8_t)> packet{'\0', '\0', PacketType::UNLOAD_FIGURE, static_cast<uint8_t>(portalSlot->getIndex())};
    return tcpSend(packet);
}

bool PortalEmulator::tcpSend(const std::span<uint8_t> packet) const
{
    packet[0] = packet.size() >> 8;
    packet[1] = packet.size();

    if (send(m_tcpClient, reinterpret_cast<const char*>(packet.data()), packet.size(), 0) != packet.size())
    {
        std::println("Failed to send data: {}", WSAGetLastError());
        return false;
    }

    return true;
}

[[nodiscard]] bool PortalEmulator::tcpReceive(const std::span<uint8_t> buffer) const
{
    const int bytesReceived{recv(m_tcpClient, reinterpret_cast<char*>(buffer.data()), buffer.size(), MSG_WAITALL)};
    const bool success{bytesReceived == buffer.size()};

    if (!success)
        std::println("TCP receive failed with code: {}", WSAGetLastError());

    return success;
}

void PortalEmulator::respond(const std::span<uint8_t> packet)
{
    if (packet[0] == PacketType::VALIDATE_PORTAL_SLOTS)
    {
        for (int i = 0; i < m_portalSlots.size(); i++)
        {
            std::unique_lock lock{m_mutex};

            const auto& portalSlot{m_portalSlots[i]};
            const auto serverState{portalSlot ? portalSlot->getState() : PortalSlotState::UNLOADED};
            const auto clientLoaded{static_cast<bool>(packet[1 + i])};

            if (serverState == PortalSlotState::LOADED && !clientLoaded)
            {
                portalSlot->setState(PortalSlotState::LOADING);
                m_sendRequests.emplace_back(std::bind_front(&PortalEmulator::requestPlayableLoad, this, std::cref(portalSlot)));
            }
            else if (serverState == PortalSlotState::UNLOADING && !clientLoaded)
                portalSlot->setState(PortalSlotState::UNLOADED);
        }
    }
    else if (packet[0] == PacketType::LOAD_FIGURE)
    {
        const auto portalSlotIndex{packet[1]};
        const auto& portalSlot{m_portalSlots[portalSlotIndex]};

        if(portalSlot->getState() != PortalSlotState::LOADING)
            std::println("ERROR: The portal slot should be in the LOADING state before transitioning to the LOADED state");

        portalSlot->setState(PortalSlotState::LOADED);
    }
    else if (packet[0] == PacketType::UNLOAD_FIGURE)
    {
        const auto portalSlotIndex{packet[1]};
        auto& portalSlot{m_portalSlots[portalSlotIndex]};

        if(portalSlot->getState() != PortalSlotState::UNLOADING)
            std::println("ERROR: The portal slot should be in the UNLOADING state before transitioning to the UNLOADED state");

        portalSlot->writeSkylanderDump(packet.subspan(2));
        portalSlot->setState(PortalSlotState::UNLOADED);
        portalSlot = nullptr;
    }
    else
    {
        std::println("ERROR: Unknown packet type received: {}", packet[0]);
        return;
    }

    {
        std::lock_guard lock{m_mutex};
        m_sendRequests.pop_front();
    }

    m_receivedTcpMessage = true;
    m_receiveCondition.notify_all();
}

void PortalEmulator::runTcpSender(const std::stop_token& token)
{
    std::println("TCP sender has started");

    while (!token.stop_requested())
    {
        std::unique_lock lock{m_mutex};
        m_sendCondition.wait(lock, [&token, this]{return token.stop_requested() || !m_sendRequests.empty();});

        if (token.stop_requested())
            break;

        const auto& request{m_sendRequests.front()};
        lock.unlock();

        if (!request())
            break;

        lock.lock();
        m_receiveCondition.wait(lock, [&token, this]{return token.stop_requested() || m_receivedTcpMessage;});

        m_receivedTcpMessage = false;

        if(token.stop_requested())
            break;
    }

    std::println("TCP sender has terminated");
    m_connected = false;
    m_disconnectionCondition.notify_all();
}

void PortalEmulator::runUdpReceiver(const std::stop_token& token)
{
    constexpr SDL_AudioSpec spec{.format = SDL_AUDIO_S16LE, .channels = 1, .freq = 8000};

    SDL_AudioStream* audioStream{SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr)};

    if (!audioStream)
        throw std::runtime_error("Failed to open audio device");

    SDL_ResumeAudioStreamDevice(audioStream);

    m_udpClient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (m_udpClient == INVALID_SOCKET)
    {
        WSACleanup();
        throw std::runtime_error("Creating socket failed with error");
    }

    constexpr DWORD TIMEOUT{3000};
    setsockopt(m_udpClient, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&TIMEOUT), sizeof(TIMEOUT));

    sockaddr_in serverAddress{};

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(UDP_PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(m_udpClient, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR)
    {
        closesocket(m_udpClient);
        WSACleanup();
        throw std::runtime_error("Bind failed with error");
    }

    sockaddr clientAddress{};
    sockaddr_in* const address{reinterpret_cast<sockaddr_in*>(&clientAddress)};

    address->sin_family = AF_INET;
    address->sin_port = htons(UDP_PORT);
    address->sin_addr.s_addr = inet_addr(PICO_IPV4_ADDRESS);

    std::println("UDP receiver has started");

    std::array<uint8_t, MUSIC_DATAGRAM_SIZE> datagram;
    auto lastSendTimePoint{std::chrono::system_clock::now().time_since_epoch()};

    while (!token.stop_requested())
    {
        const auto bytesReceived{recvfrom(m_udpClient, reinterpret_cast<char*>(datagram.data()), datagram.size(), 0, nullptr, nullptr)};

        if (bytesReceived <= 0)
        {
            std::println("UDP receive failed with code: {}", WSAGetLastError());
            break;
        }

        if (bytesReceived == 1)
            continue;

        if (constexpr int MIN_AUDIO_BYTES_QUEUED{128}; SDL_GetAudioStreamQueued(audioStream) < MIN_AUDIO_BYTES_QUEUED)
        {
            constexpr std::array<uint8_t, 4 * 1024> SILENT_AUDIO{};
            SDL_PutAudioStreamData(audioStream, SILENT_AUDIO.data(), SILENT_AUDIO.size());
        }

        if (!SDL_PutAudioStreamData(audioStream, datagram.data(), bytesReceived))
            std::println("Failed to put audio stream data: {}", SDL_GetError());

        constexpr auto SEND_DELAY{std::chrono::milliseconds(10)};
        const auto now{std::chrono::system_clock::now().time_since_epoch()};

        if (now - lastSendTimePoint > SEND_DELAY)
        {
            const int bytesQueued{SDL_GetAudioStreamQueued(audioStream)};
            sendto(m_udpClient, reinterpret_cast<const char*>(&bytesQueued), sizeof(bytesQueued), 0, &clientAddress, sizeof(clientAddress));
            lastSendTimePoint = now;
        }
    }

    SDL_DestroyAudioStream(audioStream);

    std::println("UDP receiver has terminated");
    m_connected = false;
    m_disconnectionCondition.notify_all();
}