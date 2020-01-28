#pragma once

#include <stdexcept>
#include <string>

#define MAKE_FIELD_EXCEPTION(base_class, class_name, msg)                      \
  struct class_name : public base_class {                                      \
    explicit class_name(const std::string &name, const std::string &field)     \
        : base_class(name, field, (msg)) {}                                    \
  };

namespace blebox::errors {

struct bad_ip : public std::invalid_argument {
  explicit bad_ip(const std::string &ip);
};

struct bad_mac : public std::invalid_argument {
  explicit bad_mac(const std::string &mac);
};

struct bad_device_config : public std::invalid_argument {
  explicit bad_device_config();
};

// base class for all exceptions
struct failure : public std::runtime_error {
  explicit failure(const std::string &msg);
};

struct rfx_mismatch : public failure {
  explicit rfx_mismatch(const std::string &expected, const std::string &actual);
};

struct ip_mismatch : public failure {
  explicit ip_mismatch(const std::string &ip,
                       const std::string &expected_unique_id,
                       const std::string &actual_type,
                       const std::string &actual_unique_id);
};

struct outdated_firmare_version : public failure {
  explicit outdated_firmare_version(const std::string &version);
};

struct outdated_app_version : public failure {
  explicit outdated_app_version(const std::string &version);
};

struct unsupported_device : public failure {
  explicit unsupported_device(const std::string &type);
};

struct unrecognized_device : public failure {
  explicit unrecognized_device() : failure("Not a BleBox device") {}
};

struct request_failed : public failure {
  // TODO: failure reason?
  explicit request_failed(const std::string &url);
};

struct response {
  struct invalid : public failure {
    explicit invalid(const std::string &msg) : failure(msg) {}
  };

  struct empty : public invalid {
    explicit empty(const std::string &path)
        : invalid(std::string("empty response :") + path) {}
  };

  struct invalid_field : public invalid {
    explicit invalid_field(const std::string &name, const std::string &field,
                           const std::string &message);
  };

  struct field {
    MAKE_FIELD_EXCEPTION(invalid_field, not_a_number, "is not a number");
    MAKE_FIELD_EXCEPTION(invalid_field, not_a_string, "is not a string");
    MAKE_FIELD_EXCEPTION(invalid_field, not_a_hex_string,
                         "is not a hex string");
    MAKE_FIELD_EXCEPTION(invalid_field, not_an_object, "is not an object");
    MAKE_FIELD_EXCEPTION(invalid_field, not_an_array, "is not an array");
    MAKE_FIELD_EXCEPTION(invalid_field, missing, "is missing");
    MAKE_FIELD_EXCEPTION(invalid_field, empty, "is empty");

    struct exceeds_max : public invalid_field {
      explicit exceeds_max(const std::string &name, const std::string &field,
                           int value, int max);
    };

    struct less_than_min : public invalid_field {
      explicit less_than_min(const std::string &name, const std::string &field,
                             int value, int min);
    };
  };
};

} // namespace blebox::errors
