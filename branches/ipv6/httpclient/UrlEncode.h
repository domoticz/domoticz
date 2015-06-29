#pragma once


class CURLEncode
{
private:
	static std::string csUnsafeString;
	static std::string decToHex(char num, int radix);
	static bool isUnsafe(char compareChar);
	static std::string convert(char val);

public:
	CURLEncode() { };
	virtual ~CURLEncode() { };
	static std::string URLEncode(const std::string &vData);
	static std::string URLDecode(const std::string &SRC);
};

