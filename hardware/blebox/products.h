#pragma once

#include <cstdint>
#include <map>
#include <string>

#include "../../json/value.h"
#include "box.h"

namespace blebox {
namespace products {

class airSensor : public box {
public:
  void setup(const Json::Value &response) final;
};

class dimmerBox : public box {
public:
  void setup(const Json::Value &response) final;
};

class gateBox : public box {
public:
  void setup(const Json::Value &response) final;
};

class gateController : public box {
public:
  void setup(const Json::Value &response) final;
};

class saunaBox : public box {
public:
  void setup(const Json::Value &response) final;
};

class shutterBox : public box {
public:
  void setup(const Json::Value &response) final;
};

class switchBoxD : public ::blebox::box {
public:
  void setup(const Json::Value &response) final;
};

class switchBox : public box {
public:
  void setup(const Json::Value &response) final;
};
class tempSensor : public box {
public:
  void setup(const Json::Value &response) final;
};

class wLightBox : public box {
public:
  void setup(const Json::Value &response) final;
};

class wLightBoxS : public box {
public:
  void setup(const Json::Value &response) final;
};
} // namespace products
} // namespace blebox
