#pragma once

#include "../json/json.h"

bool ParseJSon(const std::string& inStr, Json::Value& json_output, std::string* errstr = nullptr);
bool ParseJSonStrict(const std::string& inStr, Json::Value& json_output, std::string* errstr = nullptr);
std::string JSonToFormatString(const Json::Value& json_input);
std::string JSonToRawString(const Json::Value& json_input);
