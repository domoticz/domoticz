/* ------------------------------------------------------------------------- */
/*
 *  unzip_stream.h
 *
 *  Copyright (c) 2004 - 2010, clown. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    - No names of its contributors may be used to endorse or promote
 *      products derived from this software without specific prior written
 *      permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  Last-modified: Wed 13 Jan 2010 05:38:00 JST
 */
/* ------------------------------------------------------------------------- */
#ifndef CLX_UNZIP_STREAM_H
#define CLX_UNZIP_STREAM_H

//#include "config.h"
#include <cstring>
#include <string>
#include <istream>
#include <unzip.h>

namespace clx {
	/* --------------------------------------------------------------------- */
	//  basic_unzip_streambuf
	/* --------------------------------------------------------------------- */
	template <
		class CharT,
		class Traits = std::char_traits<CharT>
	>
	class basic_unzip_streambuf : public std::basic_streambuf<CharT, Traits> {
	public:
		typedef unzFile handler_type;
		typedef CharT char_type;
		typedef Traits traits;
		typedef std::vector<CharT> container;
		typedef typename container::size_type size_type;
		typedef typename Traits::int_type int_type;
		
		enum { npback = 8 };
		
		explicit basic_unzip_streambuf(handler_type h, size_type n) :
			super(), handler_(h), buffer_(n, 0) {
			this->setg(&buffer_.at(0), &buffer_.at(npback), &buffer_.at(npback));
		}
		
		virtual ~basic_unzip_streambuf() throw() {}
		
	protected:
		virtual int_type underflow() {
			if (this->gptr() == this->egptr()) {
				int_type n = static_cast<int_type>(this->gptr() - this->eback());
				if (n > npback) n = npback;
				std::memcpy(&buffer_.at(npback - n), this->gptr() - n, n);
				
				int_type byte = static_cast<int_type>((buffer_.size() - npback) * sizeof(char_type));
				int l = unzReadCurrentFile(handler_, reinterpret_cast<char*>(&buffer_.at(npback)), byte - 1);
				if (l <= 0) return traits::eof();
				this->setg(&buffer_.at(npback - n), &buffer_.at(npback), &buffer_.at(npback + l));
				return traits::to_int_type(*this->gptr());
			}
			else return traits::eof();
		}
		
		virtual int_type uflow() {
			int_type c = this->underflow();
			if (!traits::eq_int_type(c, traits::eof())) this->gbump(1);
			return c;
		}
		
	private:
		typedef std::basic_streambuf<CharT, Traits> super;
		
		handler_type handler_;
		container buffer_;
	};
	
	/* --------------------------------------------------------------------- */
	//  basic_unzip_stream
	/* --------------------------------------------------------------------- */
	template <
		class CharT,
		class Traits = std::char_traits<CharT>
	>
	class basic_unzip_stream : public std::basic_istream<CharT, Traits> {
	public:
		typedef unzFile handler_type;
		typedef CharT char_type;
		typedef std::basic_string<CharT, Traits> string_type;
		typedef basic_unzip_streambuf<CharT, Traits> streambuf_type;
		typedef typename streambuf_type::size_type size_type;
		
		enum { nbuf = 65536 };
		
		explicit basic_unzip_stream(handler_type h, size_type n = nbuf) :
			super(0), sbuf_(h, n), handler_(NULL), path_() {
			this->rdbuf(&sbuf_);
			this->open(h);
		}
		
		virtual ~basic_unzip_stream() throw() {
			this->close();
		}
		
		const string_type& path() const { return path_; }
		
	private:
		typedef std::basic_istream<CharT, Traits> super;
		
		streambuf_type sbuf_;
		handler_type handler_;
		string_type path_;
		
		basic_unzip_stream& open(handler_type h) {
			handler_ = h;
			if (handler_) {
				char path[65536];
				unz_file_info info;
				unzGetCurrentFileInfo(handler_, &info, path, sizeof(path), NULL, 0, NULL, 0);
				path_ = path;
			}
			return *this;
		}
		
		void close() {
			this->rdbuf(0); 
			path_.clear();
		}
	};
}

#endif // CLX_UNZIP_STREAM_H
