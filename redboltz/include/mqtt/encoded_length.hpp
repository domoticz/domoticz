// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_ENCODED_LENGTH_HPP)
#define MQTT_ENCODED_LENGTH_HPP

#include <string>

namespace mqtt {

inline std::string encoded_length(std::string const& str) {
    std::size_t size = str.size();
    std::string result(2, 0); // [0, 0]
    result[0] = static_cast<char>(size >> 8);
    result[1] = static_cast<char>(size & 0xff);
    return result;
}

} // namespace mqtt

#endif // MQTT_ENCODED_LENGTH_HPP
