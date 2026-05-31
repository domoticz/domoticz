#include "stdafx.h"
#include "HTMLSanitizer.h"
#include "Helper.h"
#include <regex>
#include <algorithm>

//Maybe the way we search for 'safe' words is to safe, but better safe than sorry
std::string HTMLSanitizer::Sanitize(const std::string& szText)
{
	if (szText.empty())
		return szText;
	// https://html5sec.org/
	const auto szForbiddenContent = std::array<std::string, 28>{
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
		"iframe",
		"meta",
		"link",
		"style",
		"xss"
	};

	std::string result = szText;

	// Remove each forbidden tag (both opening and closing tags)
	for (const auto& tag : szForbiddenContent) {
		// Create regex patterns for opening and closing tags
		// Matches: <tag>, <tag attr="value">, </tag>
		std::string pattern = "<\\s*/?\\s*" + std::string(tag) +
			"(?:\\s+[^>]*)?>";

		std::regex tagRegex(pattern, std::regex::icase);
		result = std::regex_replace(result, tagRegex, "");
	}
	if (result.empty())
		return "Invalid!?";

	result = StripDangerousAttributes(result);

	return result;
}

// Strips on* event handlers (quoted and unquoted) and javascript: protocol
std::string HTMLSanitizer::StripDangerousAttributes(const std::string& szText)
{
	std::string result = szText;
	// Match both quoted (onclick="...") and unquoted (onclick=foo) attribute values
	std::regex eventRegex("\\s+on\\w+\\s*=\\s*(?:[\"'][^\"']*[\"']|[^\\s>]+)", std::regex::icase);
	result = std::regex_replace(result, eventRegex, "");
	std::regex jsProtocol("javascript:", std::regex::icase);
	result = std::regex_replace(result, jsProtocol, "");
	return result;
}

// For text sensor devices: preserve HTML tags, strip only dangerous attributes/protocols
std::string HTMLSanitizer::SanitizeHTML(const std::string& szText)
{
	if (szText.empty())
		return szText;
	return StripDangerousAttributes(szText);
}
