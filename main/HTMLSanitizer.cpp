#include "stdafx.h"
#include "HTMLSanitizer.h"
#include "Helper.h"
#include <regex>
#include <algorithm>

//Maybe the way we search for 'safe' words is to safe, but better safe than sorry
std::string HTMLSanitizer::Sanitize(const std::string& szText)
{
	// https://html5sec.org/
	const auto szForbiddenContent = std::array<std::string, 27>{
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
		"style"
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

	// Also remove on* event handlers (onclick, onerror, etc.)
	std::regex eventRegex("\\s+on\\w+\\s*=\\s*[\"'][^\"']*[\"']",
		std::regex::icase);
	result = std::regex_replace(result, eventRegex, "");

	// Remove javascript: protocol
	std::regex jsProtocol("javascript:", std::regex::icase);
	result = std::regex_replace(result, jsProtocol, "");

	return result;
}

