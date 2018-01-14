// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_PUBLISH_HPP)
#define MQTT_PUBLISH_HPP

#include <cstdint>

namespace mqtt {

namespace publish {

inline
constexpr bool is_dup(std::uint8_t v) {
    return (v & 0b00001000) != 0;
}

inline
constexpr std::uint8_t get_qos(std::uint8_t v) {
    return (v & 0b00000110) >> 1;
}

constexpr bool is_retain(std::uint8_t v) {
    return (v & 0b00000001) != 0;
}

} // namespace publish

} // namespace mqtt

#endif // MQTT_PUBLISH_HPP
