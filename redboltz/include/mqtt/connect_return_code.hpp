// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_CONNECT_RETURN_CODE_HPP)
#define MQTT_CONNECT_RETURN_CODE_HPP

#include <cstdint>

namespace mqtt {

namespace connect_return_code {

constexpr std::uint8_t const accepted                      = 0;
constexpr std::uint8_t const unacceptable_protocol_version = 1;
constexpr std::uint8_t const identifier_rejected           = 2;
constexpr std::uint8_t const server_unavailable            = 3;
constexpr std::uint8_t const bad_user_name_or_password     = 4;
constexpr std::uint8_t const not_authorized                = 5;

} // namespace connect_return_code

} // namespace mqtt

#endif // MQTT_CONNECT_RETURN_CODE_HPP
