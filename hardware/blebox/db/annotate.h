#pragma once

#include <string>

namespace blebox {
namespace db {
auto annotate(const char *func_name, const char *query) -> std::string;
}
} // namespace blebox
