#pragma once
class HTMLSanitizer
{
public:
	static std::string Sanitize(const std::string& szText);
	static std::string SanitizeHTML(const std::string& szText);
	static std::string StripDangerousAttributes(const std::string& szText);
};

