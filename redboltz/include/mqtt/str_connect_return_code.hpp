// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_STR_CONNECT_RETURN_CODE_HPP)
#define MQTT_STR_CONNECT_RETURN_CODE_HPP

#include <cstdint>

namespace mqtt {

inline
char const* connect_return_code_to_str(std::uint8_t v) {
    char const * const str[] = {
        "accepted",
        " unacceptable_protocol_version",
        "identifier_rejected",
        "server_unavailable",
        "bad_user_name_or_password",
        "not_authorized"
    };
    if (v < sizeof(str)) return str[v];
    return "unknown_connect_return_code";
}

} // namespace mqtt

#endif // MQTT_STR_CONNECT_RETURN_CODE_HPP
