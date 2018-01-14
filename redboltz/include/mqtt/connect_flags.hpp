// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_CONNECT_FLAGS_HPP)
#define MQTT_CONNECT_FLAGS_HPP

#include <cstdint>

namespace mqtt {

namespace connect_flags {

constexpr char const clean_session  = 0b00000010;
constexpr char const will_flag      = 0b00000100;
constexpr char const will_retain    = 0b00100000;
constexpr char const password_flag  = 0b01000000;
constexpr char const user_name_flag = static_cast<char>(0b10000000u);

inline constexpr bool has_clean_session(char v) {
    return (v & clean_session) != 0;
}

inline constexpr bool has_will_flag(char v) {
    return (v & will_flag) != 0;
}

inline constexpr bool has_will_retain(char v) {
    return (v & will_retain) != 0;
}

inline constexpr bool has_password_flag(char v) {
    return (v & password_flag) != 0;
}

inline constexpr bool has_user_name_flag(char v) {
    return (v & user_name_flag) != 0;
}

inline void set_will_qos(char& v, std::size_t qos) {
    v |= (qos & 0b00000011) << 3;
}

inline constexpr char will_qos(char v) {
    return (v & 0b00011000) >> 3;
}

} // namespace connect_flags

} // namespace mqtt

#endif // MQTT_CONNECT_FLAGS_HPP
