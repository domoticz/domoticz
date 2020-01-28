#pragma once

#include <boost/asio/ip/address_v4.hpp>

namespace blebox {
// for convenience and to overload if needed
using boost::asio::ip::make_address_v4;

using ip = boost::asio::ip::address_v4;
} // namespace blebox
