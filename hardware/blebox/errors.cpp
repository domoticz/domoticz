#include <boost/format.hpp>
#include <string>

#include "errors.h"

namespace blebox::errors {
using namespace std::literals;

failure::failure(const std::string &msg) : std::runtime_error(msg) {}

// invalid arguments

bad_ip::bad_ip(const std::string &ip)
    : std::invalid_argument("invalid ip address: "s + ip) {}

bad_mac::bad_mac(const std::string &mac)
    : std::invalid_argument("invalid unique BleBox id: "s + mac) {}

bad_device_config::bad_device_config()
    : std::invalid_argument("invalid blebox device config") {}

// general / uncategorized

rfx_mismatch::rfx_mismatch(const std::string &expected,
                           const std::string &actual)
    : failure((boost::format("rfx id mismatch (expected %s, got %s)") %
               expected % actual)
                  .str()) {}

ip_mismatch::ip_mismatch(const std::string &ip,
                         const std::string &expected_unique_id,
                         const std::string &actual_type,
                         const std::string &actual_unique_id)
    : failure(
          (boost::format("Unexpected %s '%s' at address: %s (expected: '%s')") %
           actual_type % actual_unique_id % ip % expected_unique_id)
              .str()) {}

outdated_firmare_version::outdated_firmare_version(const std::string &version)
    : failure("unsupported/outdated firmware: "s + version) {}

outdated_app_version::outdated_app_version(const std::string &version)
    : failure("unsupported device version: "s + version +
              " (Check for Domoticz updates?)"s) {}

unsupported_device::unsupported_device(const std::string &type)
    : failure("Unrecognized device type:"s + type) {}

// invalid requests

request_failed::request_failed(const std::string &url)
    : failure("request failed to: "s + url) {}

// invalid responses

response::invalid_field::invalid_field(const std::string &name,
                                       const std::string &field,
                                       const std::string &message)
    : invalid(name + "."s + field + " "s + message) {}

response::field::exceeds_max::exceeds_max(const std::string &name,
                                          const std::string &field, int value,
                                          int max)
    : invalid_field(name, field,
                    (boost::format("%d exceeds max (%d") % value % max).str()) {
}

response::field::less_than_min::less_than_min(const std::string &name,
                                              const std::string &field,
                                              int value, int min)
    : invalid_field(
          name, field,
          (boost::format("%d less than min (%d)") % value % min).str()) {}

} // namespace blebox::errors
