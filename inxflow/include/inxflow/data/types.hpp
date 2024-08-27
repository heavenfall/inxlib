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

#ifndef INXFLOW_DATA_TYPES_HPP
#define INXFLOW_DATA_TYPES_HPP

#include <filesystem>
#include <inxlib/inx.hpp>
#include <istream>
#include <ostream>

namespace inx::flow::data {

enum class StreamType : uint8 {
	File = 1 << 0,     // stream not open, use fname
	Stream = 1 << 1,   // stream is a user-provided stream
	StdOut = 1 << 2,   // stream is std::cout
	StdIn = 1 << 3,    // stream is std::cin
	DevNull = 1 << 4,  // stream is null
};

enum class SerMode {
	Auto,  // auto deduce from ser_save_mode(), ser_load_mode(), ser_binary() in
	       // order, defaults to text otherwise
	Text,
	Binary
};

namespace concepts {

template <typename T>
concept ser_load_full =
    requires(T& t, std::istream& stream, const std::filesystem::path& path) {
	    t.load(path, stream);
    };
template <typename T>
concept ser_load_full_type =
    requires(T& t, std::istream& stream, const std::filesystem::path& path,
             StreamType type) { t.load(path, stream, type); };
template <typename T>
concept ser_load_stream =
    requires(T& t, std::istream& stream) { t.load(stream); };
template <typename T>
concept ser_load_stream_type = requires(
    T& t, std::istream& stream, StreamType type) { t.load(stream, type); };
template <typename T>
concept ser_load_filename =
    requires(T& t, const std::filesystem::path& path) { t.load(path); };
template <typename T>
concept ser_load =
    ser_load_full<T> || ser_load_full_type<T> || ser_load_stream<T> ||
    ser_load_stream_type<T> || ser_load_filename<T>;

template <typename T>
concept ser_save_full =
    requires(T& t, std::istream& stream, const std::filesystem::path& path) {
	    t.save(path, stream);
    };
template <typename T>
concept ser_save_full_type =
    requires(T& t, std::istream& stream, const std::filesystem::path& path,
             StreamType type) { t.save(path, stream, type); };
template <typename T>
concept ser_save_stream =
    requires(T& t, std::istream& stream) { t.save(stream); };
template <typename T>
concept ser_save_stream_type = requires(
    T& t, std::istream& stream, StreamType type) { t.save(stream, type); };
template <typename T>
concept ser_save_filename =
    requires(T& t, const std::filesystem::path& path) { t.save(path); };
template <typename T>
concept ser_save =
    ser_save_full<T> || ser_save_full_type<T> || ser_save_stream<T> ||
    ser_save_stream_type<T> || ser_save_filename<T>;

}  // namespace concepts

}  // namespace inx::flow::data

#endif  // INXFLOW_DATA_TYPES_HPP
