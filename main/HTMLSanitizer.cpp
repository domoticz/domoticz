#include "stdafx.h"
#include "HTMLSanitizer.h"
#include "Helper.h"

namespace
{
	// https://html5sec.org/
	const auto szForbiddenContent = std::array<std::string, 28>{
		"script",		 // noscript/onbeforescriptexecute
		"style",		 //
		"svg",			 //
		"audio",		 //
		"video",		 //
		"head",			 //
		"math",			 //
		"template",		 //
		"form",			 // formaction
		"input",		 // oninput
		"onerror",		 //
		"frame",		 // iframe/noframe/frameset
		"img",			 //
		"marquee",		 //
		"applet",		 //
		"object",		 //
		"embed",		 //
		"math",			 //
		"href",			 //
		"onfocus",		 //
		"onresize",		 //
		"onactivate",		 //
		"onscroll",		 //
		"onwebkittransitionend", //
		"onanimationstart",	 //
		"ontoggle",		 //
		"details",		 //
		"table",		 //
	};
} // namespace

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

		bool bHaveForbiddenTag = std::any_of(szForbiddenContent.begin(), szForbiddenContent.end(), [&](const std::string &content) { return tag.find(content) != std::string::npos; });
		if (!bHaveForbiddenTag)
			ret += org_tag;
	} while (true);
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

		bool bHaveForbiddenTag = std::any_of(szForbiddenContent.begin(), szForbiddenContent.end(), [&](const std::string &content) {
			std::wstring wsTmp(content.begin(), content.end());
			return tag.find(wsTmp) != std::wstring::npos;
		});

		if (!bHaveForbiddenTag)
			ret += org_tag;
	} while (true);
	//will never be reached
	return ret;
}
