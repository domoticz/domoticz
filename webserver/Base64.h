#pragma once
#ifndef _BASE64_HH
#define _BASE64_HH

#include <string>

std::string base64_encode_buf(unsigned char const* , unsigned int len);
std::string base64_encode(std::string const& s);
std::string base64_decode(std::string const& s);

std::string base64url_encode_buf(unsigned char const* , unsigned int len);
std::string base64url_encode(std::string const& s);
std::string base64url_decode(std::string const& s);

#endif
