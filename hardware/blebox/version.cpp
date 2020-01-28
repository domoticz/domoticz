#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "version.h"

blebox::version::version(const std::string &raw) {
  using namespace std::literals;

  std::vector<std::string> parts;
  boost::split(parts, raw, boost::is_any_of("."));
  if (parts.size() != 3)
    throw std::invalid_argument("bad version number: "s + raw);

  try {
    _major = std::stoi(parts[0]);
    _minor = std::stoi(parts[1]);
    _patch = std::stoi(parts[2]);
  } catch (std::invalid_argument &) {
    throw std::invalid_argument("bad version number: "s + raw);
  }
}

blebox::version::operator std::string() const {
  using namespace std::literals;
  return std::to_string(_major) + "."s + std::to_string(_minor) + "."s +
         std::to_string(_patch);
}

bool blebox::version::compatible_with(const version &minimum) {
  using namespace std::literals;

  if (_major != minimum.major())
    return false;

  if (_minor < minimum.minor())
    return false;

  if (_minor > minimum.minor())
    return true;

  return _patch >= minimum.patch();
}
