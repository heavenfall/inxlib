/*
MIT License

Copyright (c) 2024 Ryan Hechenberger

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef INXLIB_IO_NULL_HPP
#define INXLIB_IO_NULL_HPP

#include <inxlib/inx.hpp>
#include <iostream>
#include <streambuf>

namespace inx::io {

template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_null_buffer final : public std::basic_streambuf<CharT, Traits>
{
public:
	using typename std::basic_streambuf<CharT, Traits>::int_type;
	using typename std::basic_streambuf<CharT, Traits>::char_type;

protected:
	int overflow(int c) override
	{
		this->setp(m_buffer.data(), m_buffer.data() + m_buffer.size());
		return c;
	}
	std::streamsize xsputn(const char*, std::streamsize n) override
	{
		return n;
	}

	int_type underflow() override { return Traits::eof(); }
	int_type uflow() override { return Traits::eof(); }
	std::streamsize xsgetn(char_type* s, std::streamsize count) override
	{
		return 0;
	}

private:
	alignas(std::basic_streambuf<CharT, Traits>)
	  std::array<CharT, sizeof(std::max_align_t) / sizeof(CharT)> m_buffer;
};

using null_buffer = basic_null_buffer<char>;
using wnull_buffer = basic_null_buffer<wchar_t>;

namespace details {

template <typename BaseStream, typename NullBuffer>
class basic_null_stream final : public BaseStream
{
public:
	basic_null_stream()
	{
		this->init(&m_buffer); // paranoid, insuring NullBuffer is init before
		                       // constructor
	}
	// ~basic_null_stream()
	// can leave destruct, as istream/ostream/iostream does not touch the rdbuff
private:
	NullBuffer m_buffer;
};

} // namespace details

template <typename CharT, typename Traits = std::char_traits<CharT>>
using basic_null_istream =
  details::basic_null_stream<std::basic_istream<CharT, Traits>,
                             basic_null_buffer<CharT, Traits>>;
template <typename CharT, typename Traits = std::char_traits<CharT>>
using basic_null_ostream =
  details::basic_null_stream<std::basic_ostream<CharT, Traits>,
                             basic_null_buffer<CharT, Traits>>;
template <typename CharT, typename Traits = std::char_traits<CharT>>
using basic_null_iostream =
  details::basic_null_stream<std::basic_iostream<CharT, Traits>,
                             basic_null_buffer<CharT, Traits>>;

using null_istream = basic_null_istream<char>;
using null_wistream = basic_null_istream<wchar_t>;
using null_ostream = basic_null_ostream<char>;
using null_wostream = basic_null_ostream<wchar_t>;
using null_iostream = basic_null_iostream<char>;
using null_wiostream = basic_null_iostream<wchar_t>;

} // namespace inx::io

#endif // INXLIB_IO_NULL_HPP
