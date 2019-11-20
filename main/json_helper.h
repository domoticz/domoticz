#pragma once

#include "../json/json.h"

bool ParseJSon(const std::string& inStr, Json::Value& json_output, std::string *errstr = nullptr)
{
	if (inStr.empty())
		return false;
	
	bool res = Json::CharReaderBuilder().newCharReader()->parse(
		reinterpret_cast<const char*>(inStr.c_str()),
		reinterpret_cast<const char*>(inStr.c_str() + inStr.size()), &json_output,
		errstr);
	return res;
}

std::string JSonToFormatString(const Json::Value& json_input)
{
	return Json::writeString(Json::StreamWriterBuilder(), json_input);
}

std::string JSonToRawString(const Json::Value& json_input)
{
	Json::StreamWriterBuilder jsonWriter;
	jsonWriter["indentation"] = "";
	std::string sresult = Json::writeString(jsonWriter, json_input);
	return sresult;
}
