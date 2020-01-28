#pragma once

namespace blebox {
class version {
public:
  version(const std::string &raw);
  operator std::string() const;

  // semver equivalent of (current ~> 1.2 && current >= 1.2.3)
  bool compatible_with(const version &minimum);

  int major() const { return _major; }
  int minor() const { return _minor; }
  int patch() const { return _patch; }

private:
  int _major;
  int _minor;
  int _patch;
};
} // namespace blebox
