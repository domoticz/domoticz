// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_FIXED_HEADER_HPP)
#define MQTT_FIXED_HEADER_HPP

#include <cstdint>
#include <mqtt/control_packet_type.hpp>

namespace mqtt {

inline constexpr
std::uint8_t make_fixed_header(std::uint8_t type, std::uint8_t flags) {
    return
        (static_cast<std::uint8_t>(type) << 4) |
        (flags & 0x0f);
}

} // namespace mqtt

#endif // MQTT_FIXED_HEADER_HPP
