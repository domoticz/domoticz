/*
 * utf.hpp
 *
 *  Created on: 10 févr. 2016
 *      Author: gaudryc
 */

#ifndef WEBSERVER_UTF_HPP_
#define WEBSERVER_UTF_HPP_

#include <string>

namespace http {
namespace server {

/**

Conversion between UTF-8 and UTF-16 strings.

UTF-8 is used by web pages.  It is a variable byte length encoding
of UNICODE characters which is independant of the byte order in a computer word.

UTF-16 is the native Windows UNICODE encoding.

The class stores two copies of the string, one in each encoding,
so should only exist briefly while conversion is done.

This is a wrapper for the WideCharToMultiByte and MultiByteToWideChar
*/
class cUTF
{
	char * myString8;			///< string in UTF-6
public:
	/// Construct from UTF-16
	explicit cUTF( const wchar_t * ws ) {
		std::string dest;
		std::wstring src=ws;
		for (size_t i = 0; i < src.size(); i++)
		{
			wchar_t w = src[i];
			if (w <= 0x7f)
				dest.push_back((char)w);
			else if (w <= 0x7ff){
				dest.push_back(0xc0 | ((w >> 6)& 0x1f));
				dest.push_back(0x80| (w & 0x3f));
			}
			else if (w <= 0xffff){
				dest.push_back(0xe0 | ((w >> 12)& 0x0f));
				dest.push_back(0x80| ((w >> 6) & 0x3f));
				dest.push_back(0x80| (w & 0x3f));
			}
			else
				dest.push_back('?');
		}
		size_t len=dest.size();
		myString8 = (char * ) malloc( len + 1 );
		if (myString8)
		{
			memcpy(myString8, dest.c_str(), len);
			*(myString8 + len) = '\0';
		}
	}
	///  Construct from UTF8
	explicit cUTF( const char * s );
	// copy constructor
	cUTF() {
		myString8 = NULL;
	}
	/// get UTF8 version
	const char * get8() {
		return myString8;
	}
	/// free buffers
	~cUTF() {
		if (myString8) {
			free(myString8);
		}
	}
};


} // namespace server
} // namespace http

#endif /* WEBSERVER_UTF_HPP_ */
