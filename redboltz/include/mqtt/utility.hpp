// Copyright Takatoshi Kondo 2016
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_UTILITY_HPP)
#define MQTT_UTILITY_HPP

#include <utility>

#if __cplusplus >= 201402L
#define MQTT_CAPTURE_FORWARD(T, v) v = std::forward<T>(v)
#define MQTT_CAPTURE_MOVE(v) v = std::move(v)
#else
#define MQTT_CAPTURE_FORWARD(T, v) v
#define MQTT_CAPTURE_MOVE(v) v
#endif

#endif // MQTT_UTILITY_HPP
