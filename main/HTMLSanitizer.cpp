#include "stdafx.h"
#include "HTMLSanitizer.h"
#include "Helper.h"

//https://html5sec.org/
const char* szForbiddenContent[] = {
	"script", //noscript/onbeforescriptexecute
	"style",
	"svg",
	"audio",
	"video",
	"head",
	"math",
	"template",
	"form", //formaction
	"input", //oninput
	"onerror",
	"frame", //iframe/noframe/frameset
	"img",
	"marquee",
	"applet",
	"object",
	"embed",
	"math",
	"href",
	"onfocus",
	"onresize",
	"onactivate",
	"onscroll",
	"onwebkittransitionend",
	"onanimationstart",
	"ontoggle",
	"details",
	"table",

	nullptr
};

//Maybe the way we search for 'safe' words is to safe, but better safe then sorry
std::string HTMLSanitizer::Sanitize(const std::string& szText)
{
	std::string ret;
	std::string tmpstr(szText);

	do
	{
		size_t pos_start = tmpstr.find('<');
		if (pos_start == std::string::npos)
		{
			ret += tmpstr;
			return ret;
		}

		ret += tmpstr.substr(0, pos_start);
		tmpstr = tmpstr.substr(pos_start);

		size_t pos_end = tmpstr.find('>');
		if (pos_end == std::string::npos)
		{
			ret += tmpstr;
			return ret;
		}

		std::string tag = tmpstr.substr(0, pos_end + 1);
		std::string org_tag(tag);
		tmpstr = tmpstr.substr(pos_end + 1);

		//See if we have a forbidden tag, if yes remove it
		stdlower(tag);

		size_t ii = 0;
		bool bHaveForbiddenTag = false;
		while (szForbiddenContent[ii] != NULL)
		{
			if (tag.find(szForbiddenContent[ii]) != std::string::npos)
			{
				bHaveForbiddenTag = true;
				break;
			}
			ii++;
		}
		if (!bHaveForbiddenTag)
			ret += org_tag;
	} while (1);
	//will never be reached
	return ret;
}

std::wstring HTMLSanitizer::Sanitize(const std::wstring& szText)
{
	std::wstring ret;
	std::wstring tmpstr(szText);

	do
	{
		size_t pos_start = tmpstr.find('<');
		if (pos_start == std::wstring::npos)
		{
			ret += tmpstr;
			return ret;
		}

		ret += tmpstr.substr(0, pos_start);
		tmpstr = tmpstr.substr(pos_start);

		size_t pos_end = tmpstr.find('>');
		if (pos_end == std::wstring::npos)
		{
			ret += tmpstr;
			return ret;
		}

		std::wstring tag = tmpstr.substr(0, pos_end + 1);
		std::wstring org_tag(tag);
		tmpstr = tmpstr.substr(pos_end + 1);

		//See if we have a forbidden tag, if yes remove it
		stdlower(tag);

		size_t ii = 0;
		bool bHaveForbiddenTag = false;
		while (szForbiddenContent[ii] != NULL)
		{
			std::string s(szForbiddenContent[ii]);
			std::wstring wsTmp(s.begin(), s.end());
			if (tag.find(wsTmp) != std::wstring::npos)
			{
				bHaveForbiddenTag = true;
				break;
			}
			ii++;
		}
		if (!bHaveForbiddenTag)
			ret += org_tag;
	} while (1);
	//will never be reached
	return ret;
}