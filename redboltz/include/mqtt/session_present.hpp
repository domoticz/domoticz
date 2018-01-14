// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_SESSION_PRESENT_HPP)
#define MQTT_SESSION_PRESENT_HPP

namespace mqtt {

inline
constexpr bool is_session_present(char v) {
    return v & 0b00000001;
}

} // namespace mqtt

#endif // MQTT_SESSION_PRESENT_HPP
