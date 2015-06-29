//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <istream>  // NOLINT(readability/streams)
// Workarounds for symbols that are missing from Leopard stdlibc++.dylib.
_GLIBCXX_BEGIN_NAMESPACE(std)
// From ostream_insert.h
template ostream& __ostream_insert(ostream&, const char*, streamsize);

#ifdef _GLIBCXX_USE_WCHAR_T
	template wostream& __ostream_insert(wostream&, const wchar_t*, streamsize);
#endif

// From ostream.tcc
template ostream& ostream::_M_insert(long);  // NOLINT(runtime/int)
template ostream& ostream::_M_insert(unsigned long);  // NOLINT(runtime/int)
template ostream& ostream::_M_insert(bool);  // NOLINT(readability/function)
#ifdef _GLIBCXX_USE_LONG_LONG
	template ostream& ostream::_M_insert(long long);  // NOLINT(runtime/int)
	template ostream& ostream::_M_insert(unsigned long long);  // NOLINT(runtime/int)
#endif
template ostream& ostream::_M_insert(double);  // NOLINT(readability/function)
template ostream& ostream::_M_insert(long double);
template ostream& ostream::_M_insert(const void*);

#ifdef _GLIBCXX_USE_WCHAR_T
	template wostream& wostream::_M_insert(long);  // NOLINT(runtime/int)
	template wostream& wostream::_M_insert(unsigned long);  // NOLINT(runtime/int)
	template wostream& wostream::_M_insert(bool);  // NOLINT(readability/function)
	#ifdef _GLIBCXX_USE_LONG_LONG
		template wostream& wostream::_M_insert(long long);  // NOLINT(runtime/int)
		template wostream& wostream::_M_insert(unsigned long long);  // NOLINT(runtime/int)
	#endif
	template wostream& wostream::_M_insert(double);  // NOLINT(readability/function)
	template wostream& wostream::_M_insert(long double);
	template wostream& wostream::_M_insert(const void*);
#endif

// From istream.tcc
template istream& istream::_M_extract(unsigned short&);  // NOLINT(runtime/int)
template istream& istream::_M_extract(unsigned int&);
template istream& istream::_M_extract(long&);  // NOLINT(runtime/int)
template istream& istream::_M_extract(unsigned long&);  // NOLINT(runtime/int)
template istream& istream::_M_extract(bool&);
#ifdef _GLIBCXX_USE_LONG_LONG
	template istream& istream::_M_extract(long long&);  // NOLINT(runtime/int)
	template istream& istream::_M_extract(unsigned long long&);  // NOLINT(runtime/int)
#endif
template istream& istream::_M_extract(float&);
template istream& istream::_M_extract(double&);
template istream& istream::_M_extract(long double&);
template istream& istream::_M_extract(void*&);

#ifdef _GLIBCXX_USE_WCHAR_T
	template wistream& wistream::_M_extract(unsigned short&);  // NOLINT(runtime/int)
	template wistream& wistream::_M_extract(unsigned int&);
	template wistream& wistream::_M_extract(long&);  // NOLINT(runtime/int)
	template wistream& wistream::_M_extract(unsigned long&);  // NOLINT(runtime/int)
	template wistream& wistream::_M_extract(bool&);
	#ifdef _GLIBCXX_USE_LONG_LONG
		template wistream& wistream::_M_extract(long long&);  // NOLINT(runtime/int)
		template wistream& wistream::_M_extract(unsigned long long&);  // NOLINT(runtime/int)
	#endif
	template wistream& wistream::_M_extract(float&);
	template wistream& wistream::_M_extract(double&);
	template wistream& wistream::_M_extract(long double&);
	template wistream& wistream::_M_extract(void*&);
#endif

_GLIBCXX_END_NAMESPACE
