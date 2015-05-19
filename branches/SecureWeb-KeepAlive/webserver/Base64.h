#ifndef _BASE64_HH
#define _BASE64_HH

#include <string>

std::string base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const& s);

#endif
