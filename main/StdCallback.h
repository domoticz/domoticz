#pragma once

#include <string>
#include <iostream>

// most code taken from https://forums.codeguru.com/showthread.php?391038-std-output-callback
template <class Elem = char, class Tr = std::char_traits<Elem> >
class callbackstream : public std::basic_streambuf<Elem, Tr>
{
public:
	// define the callback type
	enum CB_TYPE {
		STD_OUT = 0x01, // std::cout
		STD_ERR = 0x02, // std::cerr
	};

	// define a callback function type
	typedef void (*pfncb)(const CB_TYPE, const Elem*, std::streamsize _Count);

protected:
	std::basic_ostream<Elem, Tr>& m_stream;
	std::streambuf* m_buf;
	pfncb                         m_cb;
	CB_TYPE m_cb_type;
public:
	callbackstream(const CB_TYPE vtype, std::ostream& stream, pfncb cb)
		: m_stream(stream), m_buf(nullptr), m_cb(cb), m_cb_type(vtype)
	{
		// redirect stream
		m_buf = m_stream.rdbuf(this);
	}

	~callbackstream()
	{
		// restore stream
		if (m_buf)
			m_stream.rdbuf(m_buf);
		m_buf = nullptr;
	}

	// override xsputn and make it forward data to the callback function
	std::streamsize xsputn(const Elem* _Ptr, std::streamsize _Count)
	{
		m_cb(m_cb_type, _Ptr, _Count);
		return _Count;
	}

	// override overflow and make it forward data to the callback function
	typename Tr::int_type overflow(typename Tr::int_type v)
	{
		Elem ch = Tr::to_char_type(v);
		m_cb(m_cb_type, &ch, 1);
		return Tr::not_eof(v);
	}
};
