// Copyright Takatoshi Kondo 2016
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_NULL_STRAND_HPP)
#define MQTT_NULL_STRAND_HPP

#include <boost/asio.hpp>

#include <mqtt/utility.hpp>

namespace mqtt {

namespace as = boost::asio;

struct null_strand {
    null_strand(as::io_service& ios) : ios_(ios) {}
    template <typename Func>
    void post(Func&& f) {
        ios_.post([MQTT_CAPTURE_FORWARD(Func, f)]{ f(); });
    }
    template <typename Func>
    void dispatch(Func&& f) {
        f();
    }
    template <typename Func>
    Func wrap(Func&& f) {
        return std::forward<Func>(f);
    }
private:
    as::io_service& ios_;
};

} // namespace mqtt

#endif // MQTT_NULL_STRAND_HPP
