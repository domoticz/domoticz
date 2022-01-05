#include "stdafx.h"
#include "json_helper.h"

bool ParseJSon(const std::string& inStr, Json::Value& json_output, std::string *errstr)
{
	if (inStr.empty())
		return false;

	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

	return reader->parse(
		reinterpret_cast<const char*>(inStr.c_str()),
		reinterpret_cast<const char*>(inStr.c_str() + inStr.size()), &json_output,
		errstr);
}

bool ParseJSonStrict(const std::string& inStr, Json::Value& json_output, std::string* errstr)
{
	if (inStr.empty())
		return false;

	Json::CharReaderBuilder builder;
	builder.strictMode(&builder.settings_);
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

	return reader->parse(
		reinterpret_cast<const char*>(inStr.c_str()),
		reinterpret_cast<const char*>(inStr.c_str() + inStr.size()), &json_output,
		errstr);
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
