#pragma once
class HTMLSanitizer
{
public:
	static std::string Sanitize(const std::string& szText);
	static std::wstring Sanitize(const std::wstring& szText);
};

