#pragma once

#include <string>

class IFTTT
{
public:
	static bool Send_IFTTT_Trigger(const std::string &eventid, const std::string &svalue1, const std::string &svalue2, const std::string &svalue3);
};


