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

#include "Common.hpp"

#include <array>
#include <span>
#include <algorithm>
#include <limits>
#include <functional>
#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>
#include <lwip/ip4_addr.h>
#include <lwip/netif.h>
#include <FreeRTOS.h>
#include <task.h>
#include <lwip/sockets.h>
#include <bsp/board_api.h>

using Message = std::array<uint8_t, 32>;
using SkylanderDump = std::array<uint8_t, FIGURE_DUMP_SIZE>;

int tcpServer{-1};
int udpServer{-1};
sockaddr udpServerAddress{};

struct PortalSlot
{
    bool Loaded{false};
    SkylanderDump skylanderDump;
};

std::array<PortalSlot, PORTAL_SLOT_COUNT> portalSlots;
Message statusMessage{'S', 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

int32_t queuedAudioBytes{};

static void sendTcp(const std::span<uint8_t> buffer)
{
    buffer[0] = buffer.size() >> 8;
    buffer[1] = buffer.size();

    lwip_send(tcpServer, buffer.data(), buffer.size(), 0);
}

[[nodiscard]] static bool receiveTcp(const std::span<uint8_t> buffer)
{
    const int bytesReceived{lwip_recv(tcpServer, buffer.data(), buffer.size(), MSG_WAITALL)};
    return bytesReceived == buffer.size();
}

static void sendStatusMessage()
{
    statusMessage[5]++; //race condition is acceptable for this array element
    tud_hid_report(0, statusMessage.data(), statusMessage.size());
}

static void sendStatusMessages(const uint8_t portalSlotIndex, const uint8_t initialPortalSlotState, const uint8_t finalPortalSlotState)
{
    constexpr int SKYLANDERS_PER_BYTE{4};
    constexpr int SKYLANDER_SIZE_BITS{2};
    const auto bitIndex{portalSlotIndex % SKYLANDERS_PER_BYTE * SKYLANDER_SIZE_BITS};
    const auto bitMask{~(0b11 << bitIndex)};
    auto& byte{statusMessage[1 + portalSlotIndex / SKYLANDERS_PER_BYTE]};

    byte = byte & bitMask | initialPortalSlotState << bitIndex;

    for (int i = 0; i < 5; i++)
    {
        sendStatusMessage();
        constexpr TickType_t DELAY{23 / portTICK_PERIOD_MS};
        vTaskDelay(DELAY);
    }

    byte = byte & bitMask | finalPortalSlotState << bitIndex;
    sendStatusMessage();
}

static void sendSkylanderLoadNotification(const uint8_t portalSlotIndex)
{
    constexpr uint8_t ADDING_SKYLANDER{0b11};
    constexpr uint8_t SKYLANDER_PRESENT{0b01};
    sendStatusMessages(portalSlotIndex, ADDING_SKYLANDER, SKYLANDER_PRESENT);
    portalSlots[portalSlotIndex].Loaded = true;
}

static void sendSkylanderUnloadNotification(const uint8_t portalSlotIndex)
{
    constexpr uint8_t REMOVING_SKYLANDER{0b10};
    constexpr uint8_t NO_SKYLANDER{0b00};
    sendStatusMessages(portalSlotIndex, REMOVING_SKYLANDER, NO_SKYLANDER);
    portalSlots[portalSlotIndex].Loaded = false;
}

static void handleUsb(void* params)
{
    board_init();
    tud_init(0);

    if(board_init_after_tusb)
        board_init_after_tusb();

    absolute_time_t lastPingSent{};

    while (true)
    {
        tud_task();

        const auto now{get_absolute_time()};

        if (constexpr absolute_time_t PING_DELAY_US{500000}; now - lastPingSent > PING_DELAY_US)
        {
            constexpr uint8_t PING{};
            lwip_sendto(udpServer, &PING, sizeof(PING), 0, &udpServerAddress, sizeof(udpServerAddress));
            lastPingSent = now;
        }
    }
}

static void handleDumpMessage(const std::span<const uint8_t> dumpMessage, const std::function<void(const std::span<const uint8_t>, Message&, SkylanderDump&, const int)>& operation)
{
    constexpr auto INDEX_BIT_MASK{0b00001111};
    constexpr auto BLOCK_SIZE{16};

    auto& skylanderDump{portalSlots[dumpMessage[1] & INDEX_BIT_MASK].skylanderDump};
    const auto blockIndex{dumpMessage[2]};
    const auto pos{blockIndex * BLOCK_SIZE};

    Message response{};
    std::copy_n(dumpMessage.data(), 3, response.data());

    operation(dumpMessage, response, skylanderDump, pos);
    tud_hid_report(0, response.data(), response.size());
}

static void handleQueryMessage(const std::span<const uint8_t> dumpMessage, Message& response, std::array<uint8_t, FIGURE_DUMP_SIZE>& skylanderDump, const int pos)
{
    constexpr int BYTES_TO_QUERY{16};
    std::copy_n(skylanderDump.data() + pos, BYTES_TO_QUERY, response.data() + 3);
}

static void handleWriteMessage(const std::span<const uint8_t> dumpMessage, Message& response, std::array<uint8_t, FIGURE_DUMP_SIZE>& skylanderDump, const int pos)
{
    constexpr int BYTES_TO_WRITE{16};
    std::copy_n(dumpMessage.data() + 3, BYTES_TO_WRITE, skylanderDump.data() + pos);
}

static void respond(std::array<uint8_t, MAX_PACKET_SIZE>& buffer)
{
    std::span packet{buffer.begin() + 2, buffer.end()};

    if (packet[0] == PacketType::VALIDATE_PORTAL_SLOTS)
    {
        for (int i = 0; i < portalSlots.size(); i++)
        {
            const auto serverState{static_cast<PortalSlotState>(packet[1 + i])};
            const auto& clientLoaded{portalSlots[i].Loaded};

            if((serverState == PortalSlotState::LOADING || serverState == PortalSlotState::UNLOADED) && clientLoaded)
                sendSkylanderUnloadNotification(i);

            packet[1 + i] = clientLoaded;
        }

        sendTcp({buffer.data(), 2 * sizeof(uint8_t) + sizeof(PacketType) + portalSlots.size()});
    }
    else if (packet[0] == PacketType::LOAD_FIGURE)
    {
        const auto portalSlotIndex{packet[1]};
        auto& skylanderDump{portalSlots[portalSlotIndex].skylanderDump};

        std::copy_n(packet.data() + 2, FIGURE_DUMP_SIZE, skylanderDump.data());

        sendSkylanderLoadNotification(portalSlotIndex);

        std::array responseBuffer{static_cast<uint8_t>('\0'), static_cast<uint8_t>('\0'), packet[0], packet[1]};
        sendTcp(responseBuffer);
    }
    else if (packet[0] == PacketType::UNLOAD_FIGURE)
    {
        const int portalSlotIndex{packet[1]};
        auto& skylanderDump{portalSlots[portalSlotIndex].skylanderDump};

        sendSkylanderUnloadNotification(portalSlotIndex);

        std::copy_n(skylanderDump.data(), FIGURE_DUMP_SIZE, packet.data() + 2);
        sendTcp(buffer);
    }
}

static void connectToServer()
{
    tcpServer = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in tcpServerAddress{};
    tcpServerAddress.sin_family = AF_INET;
    tcpServerAddress.sin_port = lwip_htons(TCP_PORT);
    tcpServerAddress.sin_addr.s_addr = ipaddr_addr(HOST_IPV4_ADDRESS);

    lwip_connect(tcpServer, reinterpret_cast<sockaddr*>(&tcpServerAddress), sizeof(tcpServerAddress));
}

static void handleSocketTraffic(void* params)
{
    if (cyw43_arch_init())
        return;

    cyw43_arch_enable_sta_mode();
    while (cyw43_arch_wifi_connect_blocking(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK)) {}

    const auto tcpipThread{xTaskGetHandle("tcpip_thread")};

    if (!tcpipThread)
        return;

    vTaskPrioritySet(tcpipThread, 10ul);
    vTaskCoreAffinitySet(tcpipThread, 0b01);

    udpServer = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    auto* const address{reinterpret_cast<sockaddr_in*>(&udpServerAddress)};
    address->sin_family = AF_INET;
    address->sin_port = lwip_htons(UDP_PORT);
    address->sin_addr.s_addr = ipaddr_addr(HOST_IPV4_ADDRESS);

    sockaddr_in bindAddress{};

    bindAddress.sin_family = AF_INET;
    bindAddress.sin_port = lwip_htons(UDP_PORT);
    bindAddress.sin_addr.s_addr = INADDR_ANY;

    lwip_bind(udpServer, reinterpret_cast<sockaddr*>(&bindAddress), sizeof(bindAddress));

    std::array<uint8_t, MAX_PACKET_SIZE> buffer;

    while (true)
    {
        connectToServer();

        while (true)
        {
            constexpr std::size_t PACKET_SIZE_BYTES{2};

            if (!receiveTcp({buffer.data(), PACKET_SIZE_BYTES}))
                break;

            const std::size_t packetSize{static_cast<std::size_t>(buffer[0] << 8 | buffer[1])};
            const std::size_t packetDataSize{packetSize - PACKET_SIZE_BYTES};

            if (const std::span packet{buffer.data() + 2, packetDataSize}; !receiveTcp(packet))
                break;

            respond(buffer);
        }

        lwip_close(tcpServer);
        tcpServer = -1;
    }
}

static void handleUdpReceiver(void* params)
{
    while (true)
    {
        if (lwip_recvfrom(udpServer, &queuedAudioBytes, sizeof(queuedAudioBytes), 0, nullptr, nullptr) > 0)
            continue;

        constexpr TickType_t DELAY{20 / portTICK_PERIOD_MS};
        vTaskDelay(DELAY);
    }
}

int main()
{
    stdio_init_all();

    TaskHandle_t usbTask;
    xTaskCreate(handleUsb, "UsbThread", configMINIMAL_STACK_SIZE, nullptr, 10ul, &usbTask);
    vTaskCoreAffinitySet(usbTask, 0b10);

    TaskHandle_t socketTask;
    xTaskCreate(handleSocketTraffic, "SocketThread", configMINIMAL_STACK_SIZE, nullptr, 10ul, &socketTask);
    vTaskCoreAffinitySet(socketTask, 0b01);

    TaskHandle_t udpReceiverTask;
    xTaskCreate(handleUdpReceiver, "UdpReceiverThread", configMINIMAL_STACK_SIZE, nullptr, 10ul, &udpReceiverTask);
    vTaskCoreAffinitySet(udpReceiverTask, 0b01);

    vTaskStartScheduler();

    return 0;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, const uint8_t* buffer, uint16_t bufsize)
{
    static absolute_time_t lastMusicDataSent{};

    if (constexpr absolute_time_t AUDIO_TIMEOUT{500000}; get_absolute_time() - lastMusicDataSent < AUDIO_TIMEOUT && bufsize == MUSIC_DATAGRAM_SIZE)
    {
        static absolute_time_t nextDatagramTimePoint{};

        constexpr int64_t AUDIO_BYTES_PER_SEC{16 * 1024};
        constexpr int64_t DATAGRAMS_PER_SEC{AUDIO_BYTES_PER_SEC / MUSIC_DATAGRAM_SIZE};
        constexpr int64_t US_PER_SEC{1000000};
        constexpr int64_t PREFERRED_US_PER_DATAGRAM{US_PER_SEC / DATAGRAMS_PER_SEC};
        constexpr int64_t PREFERRED_BYTES_QUEUED{4 * 1024};
        constexpr int64_t SLOPE{5};
        constexpr int64_t Y_INTERCEPT{PREFERRED_US_PER_DATAGRAM - SLOPE * PREFERRED_BYTES_QUEUED};
        constexpr int64_t MAX_US_PER_DATAGRAM{100000};
        const int64_t usPerDatagram{std::clamp(SLOPE * queuedAudioBytes + Y_INTERCEPT, 0ll, MAX_US_PER_DATAGRAM)};
        const auto currentDatagramTimePoint{nextDatagramTimePoint};

        nextDatagramTimePoint = make_timeout_time_us(static_cast<uint64_t>(usPerDatagram));
        lwip_sendto(udpServer, buffer, bufsize, 0, &udpServerAddress, sizeof(udpServerAddress));
        sleep_until(currentDatagramTimePoint);
        lastMusicDataSent = get_absolute_time();
        return;
    }

    if (buffer[0] == 'A')
    {
        Message message{'A', buffer[1], 0xFF, 0x77};
        tud_hid_report(0, message.data(), message.size());
    }
    else if (buffer[0] == 'R')
    {
        constexpr Message message{'R', 0x02, 0x18};
        tud_hid_report(0, message.data(), message.size());
    }
    else if (buffer[0] == 'J')
    {
        constexpr Message message{'J'};
        tud_hid_report(0, message.data(), message.size());
    }
    else if (buffer[0] == 'M')
    {
        const Message message{'M', buffer[1], 0x00, 0x19};
        tud_hid_report(0, message.data(), message.size());
        lastMusicDataSent = get_absolute_time();
    }
    else if (buffer[0] == 'Q')
        handleDumpMessage({buffer, bufsize}, &handleQueryMessage);
    else if (buffer[0] == 'W')
        handleDumpMessage({buffer, bufsize}, &handleWriteMessage);
    else if (buffer[0] == 'S')
        sendStatusMessage();
}