#include <cstddef>
#include <string>

#include "annotate.h"

std::string blebox::db::annotate(const char *func_name, const char *query) {
  std::string name(func_name);

  size_t params_start = name.find('(');
  size_t name_start = name.rfind(' ', params_start);
  if (name_start != std::string::npos)
    name = name.substr(name_start + 1, params_start - name_start - 1) + "()";

  using namespace std::literals;
  return "/* "s + std::string(name) + " */ "s + query;
}
