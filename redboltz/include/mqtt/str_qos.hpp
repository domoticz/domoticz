// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_STR_QOS_HPP)
#define MQTT_STR_QOS_HPP

#include <cstdint>

namespace mqtt {

namespace qos {

inline
char const* to_str(std::uint8_t v) {
    char const * const str[] = {
        "at_most_once",
        "at_least_once",
        "exactly_once"
    };
    if (v < sizeof(str)) return str[v];
    return "invalid_qos";
}

} // namespace qos

} // namespace mqtt

#endif // MQTT_STR_QOS_HPP
