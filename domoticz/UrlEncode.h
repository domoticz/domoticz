#pragma once


class CURLEncode
{
private:
	static std::string csUnsafeString;
	std::string decToHex(char num, int radix);
	bool isUnsafe(char compareChar);
	std::string convert(char val);

public:
	CURLEncode() { };
	virtual ~CURLEncode() { };
	std::string URLEncode(std::string vData);
	static std::string URLDecode(const std::string SRC);
};

