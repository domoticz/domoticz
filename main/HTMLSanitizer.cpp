#include "stdafx.h"
#include "HTMLSanitizer.h"
#include "Helper.h"

namespace
{
	// https://html5sec.org/
	const auto szForbiddenContent = std::array<std::string, 23>{
		"a",
		"span",
		"script",
		"style",
		"svg",
		"audio",
		"video",
		"head",
		"math",
		"template",
		"form",
		"input",
		"frame",
		"img",
		"marquee",
		"applet",
		"object",
		"embed",
		"math",
		"href",
		"details",
		"table",
		"alert",
	};
} // namespace

//Maybe the way we search for 'safe' words is to safe, but better safe than sorry
std::string HTMLSanitizer::Sanitize(const std::string& szText)
{
	if (szText.empty())
		return szText;

	std::string ret;
	std::string tmpstr(szText);

	do
	{
		size_t pos_start = tmpstr.find('<');
		if (pos_start == std::string::npos)
		{
			//Done
			ret += tmpstr;
			break;
		}
		std::string tag = tmpstr.substr(pos_start);
		tmpstr = tmpstr.substr(0, pos_start);
		ret += tmpstr;

		pos_start = tag.find(' ');
		if (pos_start == std::string::npos)
		{
			//Done
			ret += tag;
			break;
		}
		tmpstr = tag.substr(pos_start);
		tag = tag.substr(1, pos_start - 1);

		bool bHaveForbiddenTag = false;
		if (!tag.empty())
		{
			//See if we have a forbidden tag, if yes remove it
			stdlower(tag);
			bHaveForbiddenTag = std::any_of(szForbiddenContent.begin(), szForbiddenContent.end(), [&](const std::string& content) { return tag == content; });
		}
		if (!bHaveForbiddenTag)
			ret += "<" + tag;
	} while (true);

	return ret;
}

