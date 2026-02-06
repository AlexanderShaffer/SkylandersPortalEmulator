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

/*
 * The following device descriptor, configuration descriptor, hid report descriptor, and string descriptors (except the serial number) were
 * copied directly from a PS3 Skylanders Traptanium Portal. Wireshark was used to capture the USB packets that contain the descriptors.
 * Wireshark website: https://www.wireshark.org/
 */

#include <tusb.h>
#include <array>
#include <algorithm>

const uint8_t* tud_descriptor_device_cb()
{
    static constexpr uint8_t DEVICE_DESCRIPTOR[]{0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0x30, 0x14, 0x50, 0x01, 0x00, 0x01, 0x01, 0x02, 0x00, 0x01};
    return DEVICE_DESCRIPTOR;
}

const uint8_t* tud_descriptor_configuration_cb(const uint8_t index)
{
    static constexpr uint8_t CONFIGURATION_DESCRIPTOR[]{0x09, 0x02, 0x29, 0x00, 0x01, 0x01, 0x00, 0x80, 0xfa, 0x09, 0x04, 0x00, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00, 0x09, 0x21, 0x11, 0x01, 0x00, 0x01, 0x22, 0x1d, 0x00, 0x07, 0x05, 0x81, 0x03, 0x40, 0x00, 0x01, 0x07, 0x05, 0x02, 0x03, 0x40, 0x00, 0x01};
    return CONFIGURATION_DESCRIPTOR;
}

const uint8_t* tud_hid_descriptor_report_cb(const uint8_t instance)
{
    static constexpr uint8_t HID_REPORT_DESCRIPTOR[]{0x06, 0x00, 0xff, 0x09, 0x01, 0xa1, 0x01, 0x19, 0x01, 0x29, 0x40, 0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08, 0x95, 0x20, 0x81, 0x00, 0x19, 0x01, 0x29, 0xff, 0x91, 0x00, 0xc0};
    return HID_REPORT_DESCRIPTOR;
}

template<typename T, size_t N>
consteval auto format(const std::array<T, N>& stringDescriptor)
{
    std::array<uint16_t, N + 1> result{};
    const uint16_t header{TUSB_DESC_STRING << 8 | sizeof(result)};

    result[0] = header;
    std::ranges::copy(stringDescriptor, std::next(result.begin()));
    return result;
}

const uint16_t* tud_descriptor_string_cb(const uint8_t index, const uint16_t languageId)
{
    static constexpr uint16_t UNITED_STATES_ENGLISH{0x0409};
    static constexpr auto SUPPORTED_LANGUAGE{format(std::array{UNITED_STATES_ENGLISH})};
    static constexpr auto MANUFACTURER{format(std::array{'A', 'c', 't', 'i', 'v', 'i', 's', 'i', 'o', 'n'})};
    static constexpr auto DEVICE_DESCRIPTION{format(std::array{'S', 'p', 'y', 'r', 'o', ' ', 'P', 'o', 'r', 't', 'a'})};
    static constexpr std::array STRING_DESCRIPTORS{SUPPORTED_LANGUAGE.data(), MANUFACTURER.data(), DEVICE_DESCRIPTION.data()};
    return index < STRING_DESCRIPTORS.size() ? STRING_DESCRIPTORS[index] : nullptr;
}