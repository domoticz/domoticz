// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_UTF8ENCODED_STRINGS_HPP)
#define MQTT_UTF8ENCODED_STRINGS_HPP

#include <string>

namespace mqtt {

namespace utf8string {

inline bool
is_valid_length(std::string const& str) {
    return str.size() <= 0xffff;
}

inline bool
is_valid_contents(std::string const& /*str*/) {
    // not implemented yet
    // See 1.5.3 UTF-8 encoded strings
    return true;
}

} // namespace utf8string

} // namespace mqtt

#endif // MQTT_UTF8ENCODED_STRINGS_HPP
