// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_HEXDUMP_HPP)
#define MQTT_HEXDUMP_HPP

#include <ostream>
#include <iomanip>

namespace mqtt {

template <typename T>
inline void hexdump(std::ostream& os, T const& v) {
    for (auto c : v) {
        os << std::hex << std::setw(2) << std::setfill('0');
        os << (static_cast<int>(c) & 0xff) << ' ';
    }
}

} // namespace mqtt

#endif // MQTT_HEXDUMP_HPP
