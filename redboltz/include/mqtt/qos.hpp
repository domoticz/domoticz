// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_QOS_HPP)
#define MQTT_QOS_HPP

#include <cstdint>

namespace mqtt {

namespace qos {

constexpr std::uint8_t const at_most_once  = 0;
constexpr std::uint8_t const at_least_once = 1;
constexpr std::uint8_t const exactly_once  = 2;

} // namespace qos

} // namespace mqtt

#endif // MQTT_QOS_HPP
