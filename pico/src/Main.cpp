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

#include <algorithm>
#include <l2cap.h>
#include <ble/att_server.h>
#include <ble/sm.h>
#include <pico/cyw43_arch.h>
#include <bluetooth.h>
#include <bluetooth_data_types.h>
#include <btstack_event.h>
#include <gap.h>
#include <server.h>
#include <pico/stdlib.h>
#include <bsp/board_api.h>
#include <array>
#include <atomic>
#include <pico/multicore.h>
#include <Common.hpp>
#include <span>

namespace
{
using Message = std::array<uint8_t, 32>;

struct PortalSlot
{
    std::atomic_bool Loaded{false};
    std::array<uint8_t, FIGURE_DUMP_SIZE> SkylanderDump;
    std::atomic_uint8_t Timer{0};
};

constexpr uint8_t MIN_TIMER_VALUE{0};
constexpr uint8_t MAX_TIMER_VALUE{15};
constexpr uint16_t MUSIC_PACKET_SIZE{32};
constexpr size_t MUSIC_NOTIFICATION_SIZE{MUSIC_PACKET_SIZE * 16};
std::array<PortalSlot, PORTAL_SLOT_COUNT> portalSlots;
std::atomic_bool connected{false};
std::atomic_int32_t queuedAudioBytes{0};
hci_con_handle_t connectionHandle{0};

void handleDumpMessage(const std::span<const uint8_t> dumpMessage, void(* const operation)(std::span<const uint8_t>, std::span<uint8_t> response, std::span<uint8_t>, int))
{
    static constexpr int INDEX_BIT_MASK{0x0F};
    static constexpr int BLOCK_SIZE{16};
    static constexpr int SUCCESS_BITS{0x10};

    const std::span skylanderDump{portalSlots[dumpMessage[1] & INDEX_BIT_MASK].SkylanderDump};
    const uint8_t blockIndex{dumpMessage[2]};
    const int pos{blockIndex * BLOCK_SIZE};

    Message response{};
    std::copy_n(dumpMessage.data(), 3, response.data());
    response[1] = (response[1] & INDEX_BIT_MASK) | SUCCESS_BITS;

    operation(dumpMessage, response, skylanderDump, pos);
    tud_hid_report(0, response.data(), response.size());
}

void handleQueryMessage(const std::span<const uint8_t> /* dumpMessage */, const std::span<uint8_t> response, std::span<uint8_t> skylanderDump, const int pos)
{
    constexpr int BYTES_TO_QUERY{16};
    std::copy_n(skylanderDump.data() + pos, BYTES_TO_QUERY, response.data() + 3);
}

void handleWriteMessage(const std::span<const uint8_t> dumpMessage, const std::span<uint8_t> /* response */, std::span<uint8_t> skylanderDump, const int pos)
{
    constexpr int BYTES_TO_WRITE{16};
    std::copy_n(dumpMessage.data() + 3, BYTES_TO_WRITE, skylanderDump.data() + pos);
}

void indicate(const std::span<uint8_t> packet)
{
    att_server_indicate(connectionHandle, ATT_CHARACTERISTIC_022dcf96_388f_4531_99ef_1cf30449ac93_01_VALUE_HANDLE, packet.data(), packet.size());
}

void respondToApp(void* context)
{
    static constexpr uint32_t SLOT_STATE_CHANGE_DELAY{500};
    uint8_t* const packet{static_cast<uint8_t*>(context)};
    const uint8_t type = packet[0];

    if (type == PacketType::VALIDATE_PORTAL_SLOTS)
    {
        for (int i = 0; i < portalSlots.size(); i++)
        {
            const auto appSlotState{static_cast<PortalSlotState>(packet[1 + i])};
            auto& loadedOnPico{portalSlots[i].Loaded};

            if((appSlotState == PortalSlotState::LOADING || appSlotState == PortalSlotState::UNLOADED) && loadedOnPico)
                loadedOnPico = false;

            packet[1 + i] = loadedOnPico;
        }

        indicate(std::span{packet, sizeof(PacketType) + portalSlots.size()});
    }
    else if (type == PacketType::LOAD_FIGURE_HALF_1 || type == PacketType::LOAD_FIGURE_HALF_2)
    {
        const uint8_t portalSlotIndex{packet[1]};
        PortalSlot& portalSlot{portalSlots[portalSlotIndex]};

        if (type == PacketType::LOAD_FIGURE_HALF_1)
            std::copy_n(packet + 2, FIGURE_HALF_DUMP_SIZE, portalSlot.SkylanderDump.data());
        else
        {
            std::copy_n(packet + 2, FIGURE_HALF_DUMP_SIZE, portalSlot.SkylanderDump.data() + FIGURE_HALF_DUMP_SIZE);
            portalSlot.Timer = MIN_TIMER_VALUE;
            portalSlot.Loaded = true;
            busy_wait_ms(SLOT_STATE_CHANGE_DELAY);
        }

        indicate(std::span{packet, 2});
    }
    else if (type == PacketType::UNLOAD_FIGURE_HALF_1 || type == PacketType::UNLOAD_FIGURE_HALF_2)
    {
        const uint8_t portalSlotIndex{packet[1]};
        PortalSlot& portalSlot{portalSlots[portalSlotIndex]};

        if (type == PacketType::UNLOAD_FIGURE_HALF_1)
        {
            portalSlot.Timer = MAX_TIMER_VALUE;
            portalSlot.Loaded = false;
            busy_wait_ms(SLOT_STATE_CHANGE_DELAY);
            std::copy_n(portalSlot.SkylanderDump.data(), FIGURE_HALF_DUMP_SIZE, packet + 2);
        }
        else
            std::copy_n(portalSlot.SkylanderDump.data() + FIGURE_HALF_DUMP_SIZE, FIGURE_HALF_DUMP_SIZE, packet + 2);

        indicate(std::span{packet, sizeof(PacketType) + sizeof(uint8_t) + FIGURE_HALF_DUMP_SIZE});
    }
}

int attWriteCallback(hci_con_handle_t handle, uint16_t attributeHandle, uint16_t transactionMode, uint16_t offset, uint8_t* buffer, uint16_t bufferSize)
{
    if (attributeHandle == ATT_CHARACTERISTIC_022dcf96_388f_4531_99ef_1cf30449ac93_01_VALUE_HANDLE)
    {
        static std::array<uint8_t, MAX_PACKET_SIZE> packet;
        static btstack_context_callback_registration_t indicationCallback{.callback = respondToApp, .context = packet.data()};

        std::copy_n(buffer, bufferSize, packet.begin());
        att_server_request_to_send_indication(&indicationCallback, connectionHandle);
    }
    else if (attributeHandle == ATT_CHARACTERISTIC_d77f953a_932d_40d4_b4cd_7310bea9c201_01_VALUE_HANDLE)
    {
        static_assert(std::endian::native == std::endian::little);
        int32_t temp;

        std::copy_n(buffer, sizeof(temp), reinterpret_cast<uint8_t*>(&temp));
        queuedAudioBytes = temp;
    }

    return 0;
}

void handleHciEvent(uint8_t packetType, uint16_t channel, uint8_t* packet, uint16_t size)
{
    if (packetType != HCI_EVENT_PACKET)
        return;

    if (const uint8_t event{hci_event_packet_get_type(packet)}; event == HCI_EVENT_META_GAP && hci_event_gap_meta_get_subevent_code(packet) == GAP_SUBEVENT_LE_CONNECTION_COMPLETE)
    {
        connectionHandle = gap_subevent_le_connection_complete_get_connection_handle(packet);
        connected = true;
        gap_advertisements_enable(false);
    }
    else if (event == HCI_EVENT_DISCONNECTION_COMPLETE)
    {
        connected = false;
        gap_advertisements_enable(true);
    }
}

void initGattServer()
{
    static btstack_packet_callback_registration_t hciEventHandler{.callback = handleHciEvent};

    hci_add_event_handler(&hciEventHandler);
    att_server_register_packet_handler(handleHciEvent);
    att_server_init(profile_data, nullptr, attWriteCallback);

    static constexpr uint8_t GENERAL_DISCOVERABILITY_FLAG{0x06};

    static uint8_t data[] =
    {
        0x02, BLUETOOTH_DATA_TYPE_FLAGS, GENERAL_DISCOVERABILITY_FLAG,
        0x14, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 's', 'k', 'y', '_', 'p', 'o', 'r', 't', 'a', 'l', '_', 'e', 'm', 'u', 'l', 'a', 't', 'o', 'r',
        0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x1a, 0x18,
    };

    static_assert(sizeof(data) <= 31);

    static constexpr uint16_t INTERVAL{1000};
    static constexpr uint8_t ADVERTISEMENT_TYPE{0};
    static constexpr uint8_t DIRECT_ADDRESS_TYPE{0};
    static constexpr uint8_t CHANNEL_MAP{0x07};
    static constexpr uint8_t FILTER_POLICY{0x00};
    bd_addr_t directAddress{};

    gap_advertisements_set_params(INTERVAL, INTERVAL, ADVERTISEMENT_TYPE, DIRECT_ADDRESS_TYPE, directAddress, CHANNEL_MAP, FILTER_POLICY);
    gap_advertisements_set_data(sizeof(data), data);
    gap_advertisements_enable(true);
}

void sendStatusMessage(bool shouldSend)
{
    static uint8_t statusId{0};
    std::array<uint8_t, 32> statusMessage{'S', 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

    for (int portalSlotIndex{0}; portalSlotIndex < portalSlots.size(); portalSlotIndex++)
    {
        static constexpr uint8_t FINAL_STATE_TIMER_OFFSET{5};
        static constexpr uint8_t LOADING_SKYLANDER{0b11};
        static constexpr uint8_t SKYLANDER_LOADED{0b01};
        static constexpr uint8_t REMOVING_SKYLANDER{0b10};
        static constexpr uint8_t NO_SKYLANDER{0b00};

        PortalSlot& portalSlot{portalSlots[portalSlotIndex]};
        uint8_t slotState;

        if (portalSlot.Loaded)
            slotState = portalSlot.Timer >= MAX_TIMER_VALUE - FINAL_STATE_TIMER_OFFSET ? SKYLANDER_LOADED : LOADING_SKYLANDER;
        else
            slotState = portalSlot.Timer <= MIN_TIMER_VALUE + FINAL_STATE_TIMER_OFFSET ? NO_SKYLANDER : REMOVING_SKYLANDER;

        static constexpr int SKYLANDERS_PER_BYTE{4};
        static constexpr int SKYLANDER_SIZE_BITS{2};
        const auto bitIndex{(portalSlotIndex % SKYLANDERS_PER_BYTE) * SKYLANDER_SIZE_BITS};
        const auto bitMask{~(0b11 << bitIndex)};
        auto& byte{statusMessage[1 + portalSlotIndex / SKYLANDERS_PER_BYTE]};

        byte = (byte & bitMask) | (slotState << bitIndex);
        shouldSend |= portalSlot.Timer != MIN_TIMER_VALUE && portalSlot.Timer != MAX_TIMER_VALUE;

        if (portalSlot.Loaded)
            portalSlot.Timer = std::min(MAX_TIMER_VALUE, static_cast<uint8_t>(portalSlot.Timer + 1));
        else
            portalSlot.Timer = portalSlot.Timer == MIN_TIMER_VALUE ? MIN_TIMER_VALUE : portalSlot.Timer - 1;
    }

    if (shouldSend)
    {
        statusMessage[5] = statusId++;
        tud_hid_report(0, statusMessage.data(), statusMessage.size());
    }
}

void handleUsb()
{
    board_init();
    tud_init(0);
    board_init_after_tusb();

    absolute_time_t timeDuringLastStatus{};

    while (true)
    {
        tud_task();

        const auto now{get_absolute_time()};

        if (constexpr absolute_time_t STATUS_DELAY_US{25000}; now - timeDuringLastStatus > STATUS_DELAY_US)
        {
            sendStatusMessage(false);
            timeDuringLastStatus = now;
        }
    }
}

void sendMusicNotification(void* context)
{
    att_server_notify(connectionHandle, ATT_CHARACTERISTIC_d77f953a_932d_40d4_b4cd_7310bea9c201_01_VALUE_HANDLE, static_cast<uint8_t*>(context), MUSIC_NOTIFICATION_SIZE);
}

void requestToSendMusicNotification(void* context)
{
    static btstack_context_callback_registration_t notificationCallback{.callback = sendMusicNotification, .context = context};
    att_server_request_to_send_notification(&notificationCallback, connectionHandle);
}

void handleMusicPacket(const uint8_t* const buffer, const absolute_time_t timeSinceLastMusic)
{
    static constexpr absolute_time_t AUDIO_ENDED_THRESHOLD{50000};
    static size_t musicDataIndex{0};

    if (timeSinceLastMusic > AUDIO_ENDED_THRESHOLD)
    {
        queuedAudioBytes = 0;
        musicDataIndex = 0;
    }

    static std::array<uint8_t, MUSIC_NOTIFICATION_SIZE> musicData;

    std::copy_n(buffer, MUSIC_PACKET_SIZE, musicData.begin() + musicDataIndex);
    musicDataIndex += MUSIC_PACKET_SIZE;

    if (musicDataIndex < MUSIC_NOTIFICATION_SIZE)
        return;

    static constexpr int64_t AUDIO_BYTES_PER_SEC{16 * 1024};
    static constexpr int64_t NOTIFICATIONS_PER_SEC{AUDIO_BYTES_PER_SEC / MUSIC_NOTIFICATION_SIZE};
    static constexpr int64_t US_PER_SEC{1000000};
    static constexpr int64_t PREFERRED_US_PER_PACKET{US_PER_SEC / NOTIFICATIONS_PER_SEC};
    static constexpr int64_t PREFERRED_BYTES_QUEUED{4 * 1024};
    static constexpr int64_t SLOPE{15};
    static constexpr int64_t Y_INTERCEPT{PREFERRED_US_PER_PACKET - SLOPE * PREFERRED_BYTES_QUEUED};
    static constexpr int64_t MAX_US_PER_PACKET{200000};
    static btstack_context_callback_registration_t runLoopCallback{.callback = requestToSendMusicNotification, .context = musicData.data()};
    static absolute_time_t nextNotificationTimePoint{};
    const absolute_time_t currentNotificationTimePoint{nextNotificationTimePoint};
    const int64_t notificationDelayUs{std::clamp(SLOPE * queuedAudioBytes + Y_INTERCEPT, 0ll, MAX_US_PER_PACKET)};

    nextNotificationTimePoint = make_timeout_time_us(static_cast<uint64_t>(notificationDelayUs));
    musicDataIndex = 0;
    btstack_run_loop_execute_on_main_thread(&runLoopCallback);
    busy_wait_until(currentNotificationTimePoint);
}
} // namespace

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, const uint8_t* buffer, uint16_t bufsize)
{
    static absolute_time_t timeDuringLastMusic{};
    static constexpr absolute_time_t AUDIO_TIMEOUT{500000};

    if (const absolute_time_t timeSinceLastMusic{get_absolute_time() - timeDuringLastMusic}; timeSinceLastMusic < AUDIO_TIMEOUT && bufsize == MUSIC_PACKET_SIZE)
    {
        if (connected)
            handleMusicPacket(buffer, timeSinceLastMusic);

        timeDuringLastMusic = get_absolute_time();
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
        timeDuringLastMusic = get_absolute_time();
    }
    else if (buffer[0] == 'Q')
        handleDumpMessage({buffer, bufsize}, handleQueryMessage);
    else if (buffer[0] == 'W')
        handleDumpMessage({buffer, bufsize}, handleWriteMessage);
    else if (buffer[0] == 'S')
        sendStatusMessage(true);
}

int main()
{
    stdio_init_all();

    if (cyw43_arch_init())
        return 1;

    l2cap_init();
    sm_init();
    initGattServer();
    hci_power_control(HCI_POWER_ON);
    multicore_launch_core1(handleUsb);

    while (true)
    {
        async_context_poll(cyw43_arch_async_context());
        async_context_wait_for_work_until(cyw43_arch_async_context(), at_the_end_of_time);
    }
}