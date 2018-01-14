// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_REMAINING_LENGTH_HPP)
#define MQTT_REMAINING_LENGTH_HPP

#include <string>
#include <mqtt/exception.hpp>

namespace mqtt {

inline std::string
remaining_bytes(std::size_t size) {
    if (size > 0xfffffff) throw remaining_length_error();
    std::string bytes;
    while (size > 127) {
        bytes.push_back((size & 0b01111111) | 0b10000000);
        size >>= 7;
    }
    bytes.push_back(size & 0b01111111);
    return bytes;
}

inline std::tuple<std::size_t, std::size_t>
remaining_length(std::string const& bytes) {
    std::size_t len = 0;
    std::size_t mul = 1;
    std::size_t consumed = 0;
    for (auto b : bytes) {
        len += (b & 0b01111111) * mul;
        mul *= 128;
        ++consumed;
        if (mul > 128 * 128 * 128 * 128) return std::make_tuple(0, 0);
        if (!(b & 0b10000000)) break;
    }
    return std::make_tuple(len, consumed);
}

} // namespace mqtt

#endif // MQTT_REMAINING_LENGTH_HPP
