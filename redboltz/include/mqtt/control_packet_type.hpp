// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_CONTROL_PACKET_TYPE_HPP)
#define MQTT_CONTROL_PACKET_TYPE_HPP

#include <cstdint>

namespace mqtt {

namespace control_packet_type {
 // reserved    =  0,
constexpr std::uint8_t const connect     =  1;
constexpr std::uint8_t const connack     =  2;
constexpr std::uint8_t const publish     =  3;
constexpr std::uint8_t const puback      =  4;
constexpr std::uint8_t const pubrec      =  5;
constexpr std::uint8_t const pubrel      =  6;
constexpr std::uint8_t const pubcomp     =  7;
constexpr std::uint8_t const subscribe   =  8;
constexpr std::uint8_t const suback      =  9;
constexpr std::uint8_t const unsubscribe = 10;
constexpr std::uint8_t const unsuback    = 11;
constexpr std::uint8_t const pingreq     = 12;
constexpr std::uint8_t const pingresp    = 13;
constexpr std::uint8_t const disconnect  = 14;
 // reserved    = 15

} // namespace control_packet_type

inline
constexpr std::uint8_t get_control_packet_type(std::uint8_t v) {
    return v >> 4;
}

} // namespace mqtt

#endif // MQTT_CONTROL_PACKET_TYPE_HPP
