#pragma once

#include <string>
#include <vector>

#include "../hardware/blebox/ip.h"

class MemDBSession {
public:
  using record_type = std::vector<std::string>;
  using results_type = std::vector<record_type>;

  MemDBSession();
  ~MemDBSession();

  auto create_blebox_hardware(const std::string &unique_id,
                              const blebox::ip &ip, const std::string &name,
                              const std::string &type) -> int;

  auto create_blebox_feature(int hw_id, const std::string &name,
                             const std::string &deviceid) -> int;
};
