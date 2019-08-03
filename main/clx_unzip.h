/* ------------------------------------------------------------------------- */
/*
 *  unzip.h
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
 *  Last-modified: Fri 30 Jul 2010 19:47:03 JST
 */
/* ------------------------------------------------------------------------- */
#ifndef CLX_UNZIP_H
#define CLX_UNZIP_H

//#include "config.h"
#include <string>
#include <ios>
//#include "config.h"
//#include "shared_ptr.h"
#include <unzip.h>
#include <boost/shared_ptr.hpp>

#ifdef CLX_ENABLE_AUTOLINK
#if defined(_MSC_VER) && (_MSC_VER >= 1200) || defined(__BORLANDC__)
#pragma comment(lib, "zlib.lib")
#endif
#endif // CLX_ENABLE_AUTOLINK

#include "unzip_stream.h"
#include "unzip_iterator.h"

namespace clx {
	/* --------------------------------------------------------------------- */
	//  basic_unzip
	/* --------------------------------------------------------------------- */
	template <
		class CharT,
		class Traits = std::char_traits<CharT>
	>
	class basic_unzip {
	public:
		typedef size_t size_type;
		typedef CharT char_type;
		typedef unzFile handler_type;
		typedef std::basic_string<CharT, Traits> string_type;
		typedef basic_unzip_stream<CharT, Traits> stream_type;
		typedef basic_unzip_iterator<CharT, Traits> iterator;
		
		basic_unzip() :
			p_(), pass_() {}
		
		basic_unzip(const basic_unzip& cp) :
			p_(cp.p_), pass_(cp.pass_) {}
		
		basic_unzip& operator=(const basic_unzip& cp) {
			if (cp.p_) p_ = cp.p_;
			return *this;
		}
		
		explicit basic_unzip(const string_type& path) :
			p_(), pass_() {
			this->open(path);
		}
		
		basic_unzip(const string_type& path, const string_type& password) :
			p_(), pass_(password) {
			this->open(path);
		}
		
		explicit basic_unzip(const char_type* path) :
			p_(), pass_() {
			this->open(path);
		}
		
		basic_unzip(const char_type* path, const char_type* password) :
			p_(), pass_() {
			this->open(path, password);
		}
		
		virtual ~basic_unzip() throw() { this->close(); }
		
		bool open(const char_type* path) {
			handler_type h = unzOpen(path);
			if (h == NULL) return false;
			p_ = boost::shared_ptr<storage_impl>(new storage_impl(h));
			return true;
		}
		
		bool open(const char_type* path, const char_type* password) {
			if (password) pass_ = password;
			return this->open(path);
		}
		
		bool open(const string_type& path) {
			return this->open(path.c_str());
		}
		
		bool open(const string_type& path, const string_type& password) {
			pass_ = password;
			return this->open(path);
		}
		
		void close() { if (p_) p_->close(); }
		
		bool is_open() const { return (p_) ? p_->is_open() : false; }
		
		bool exist(const char_type* path) const {
			return (p_ && unzLocateFile(p_->handler(), path, 0) == UNZ_OK);
		}
		
		bool exist(const string_type& path) const {
			return this->exist(path.c_str());
		}
		
		iterator begin() {
			if (!this->is_open() || unzGoToFirstFile(p_->handler()) != UNZ_OK) {
				return iterator();
			}
			return iterator(*this);
		}
		
		iterator end() { return iterator(); }
		
		iterator find(const char_type* path) {
			if (!this->is_open() || !this->exist(path)) return iterator();
			return iterator(*this);
		}
		
		iterator find(const string_type& path) {
			return this->find(path.c_str());
		}
		
		handler_type handler() const { return (p_) ? p_->handler() : NULL; }
		const string_type& password() const { return pass_; }
		
	private:
		class storage_impl {
		public:
			explicit storage_impl(handler_type in) : in_(in) {}
			~storage_impl() { this->close(); }
			
			void close() {
				if (in_ != NULL) {
					unzClose(in_);
					in_ = NULL;
				}
			}
			
			handler_type handler() { return in_; }
			bool is_open() const { return (in_ != NULL); }
			
		private:
			handler_type in_;
		};
		
		boost::shared_ptr<storage_impl> p_;
		string_type pass_;
	};
	
	typedef basic_unzip<char> unzip;
}

#endif // CLX_UNZIP_H
