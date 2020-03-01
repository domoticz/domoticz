#pragma once

#ifdef WITH_EXTERNAL_JSONCPP
#include <json/json.h>
#else
#include "../json/json.h"
#endif

bool ParseJSon(const std::string& inStr, Json::Value& json_output, std::string* errstr = nullptr);
bool ParseJSonStrict(const std::string& inStr, Json::Value& json_output, std::string* errstr = nullptr);
std::string JSonToFormatString(const Json::Value& json_input);
std::string JSonToRawString(const Json::Value& json_input);
