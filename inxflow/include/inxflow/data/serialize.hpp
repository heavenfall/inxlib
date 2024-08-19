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

#ifndef INXFLOW_DATA_SERIALIZE_HPP
#define INXFLOW_DATA_SERIALIZE_HPP

#include <inxlib/types.hpp>
#include <inxlib/util/functions.hpp>
#include <inxlib/io/null.hpp>
#include "types.hpp"
#include <iostream>
#include <typeindex>

namespace inx::flow::data
{

namespace details {
template <ser_load T>
bool serialize_load_path_stream(T& data, const std::filesystem::path& path, std::istream& in, StreamType stype)
{
	if constexpr (ser_load_full_type<T>) {
		data.load(path, in, stype);
		return true;
	} else if constexpr (ser_load_full<T>) {
		data.load(path, in);
		return true;
	} else if constexpr (ser_load_stream_type<T>) {
		data.load(in, stype);
		return true;
	} else if constexpr (ser_load_stream<T>) {
		data.load(in);
		return true;
	}
	return false;
}
template <ser_load T>
bool serialize_load_stream(T& data, std::istream& in, StreamType stype)
{
	if constexpr (ser_load_stream_type<T>) {
		data.load(in, stype);
		return true;
	} else if constexpr (ser_load_stream<T>) {
		data.load(in);
		return true;
	} else if constexpr (ser_load_full_type<T>) {
		data.load({}, in, stype);
		return true;
	} else if constexpr (ser_load_full<T>) {
		data.load({}, in);
		return true;
	}
	return false;
}
template <ser_load T>
bool serialize_load_file(T& data, const std::filesystem::path& path, std::ios_base::openmode mode)
{
	if constexpr (ser_load_filename<T>) {
		return data.load(path);
	} else {
		std::ifstream in(path, mode);
		if (!in)
			return false;
		return serialize_load_path_stream(data, path, in, StreamType::File);
	}
}

template <ser_save T>
bool serialize_save_path_stream(T& data, const std::filesystem::path& path, std::ostream& out, StreamType stype)
{
	if constexpr (ser_save_full_type<T>) {
		data.save(path, out, stype);
		return true;
	} else if constexpr (ser_save_full<T>) {
		data.save(path, out);
		return true;
	} else if constexpr (ser_save_stream_type<T>) {
		data.save(out, stype);
		return true;
	} else if constexpr (ser_save_stream<T>) {
		data.save(out);
		return true;
	}
	return false;
}
template <ser_save T>
bool serialize_save_stream(T& data, std::istream& in, StreamType stype)
{
	if constexpr (ser_save_stream_type<T>) {
		data.save(in, stype);
		return true;
	} else if constexpr (ser_save_stream<T>) {
		data.save(in);
		return true;
	} else if constexpr (ser_save_full_type<T>) {
		data.save({}, in, stype);
		return true;
	} else if constexpr (ser_save_full<T>) {
		data.save({}, in);
		return true;
	}
	return false;
}
template <ser_save T>
bool serialize_save_file(T& data, const std::filesystem::path& path, std::ios_base::openmode mode)
{
	if constexpr (ser_save_filename<T>) {
		return data.save(path);
	} else {
		std::ifstream in(path, mode);
		if (!in)
			return false;
		return serialize_save_path_stream(data, path, in, StreamType::File);
	}
}
} // namespace details

template <ser_load T>
bool serialize_load(T& data, const std::filesystem::path& path, StreamType stype, std::ios_base::openmode mode = std::ios::in)
{
	switch (stype) {
	case StreamType::StdIn:
		return details::serialize_load_stream(data, std::cin, StreamType::StdIn);
	case StreamType::DevNull: {
		inx::io::null_istream nulls;
		return details::serialize_load_stream(data, nulls, StreamType::DevNull);
	}
	case StreamType::File:
		return details::serialize_load_file(data, path, mode);
	case StreamType::StdOut:
	case StreamType::Var:
	default:
		return false;
	}
}

template <ser_save T>
bool serialize_save(T& data, const std::filesystem::path& path, StreamType stype, std::ios_base::openmode mode = std::ios::out | std::ios::trunc)
{
	switch (stype) {
	case StreamType::StdOut:
		return details::serialize_save_stream(data, std::cout, StreamType::StdOut);
	case StreamType::DevNull: {
		inx::io::null_ostream nulls;
		return details::serialize_save_stream(data, nulls, StreamType::DevNull);
	}
	case StreamType::File:
		return details::serialize_save_file(data, path, mode);
	case StreamType::StdIn:
	case StreamType::Var:
	default:
		return false;
	}
}

//
// SerializeWrapper
//

enum class SerMode {
	Auto, // auto deduce from ser_save_mode(), ser_load_mode(), ser_binary() in order, defaults to text otherwise
	Text,
	Binary
};

namespace concepts {
template <typename T>
concept has_ser_binary = requires {
	T::ser_binary();
};
template <typename T>
concept has_ser_load_mode = requires {
	T::ser_load_mode();
};
template <typename T>
concept has_ser_save_mode = requires {
	T::ser_save_mode();
};
}
class SerializeWrapper
{
public:
	enum class wrapper_op : uint8 {
		Construct, // (SerializeWrapper*)
		Load, // (T*, std::istream*, const std::filesystem::path*, StreamType)
		Save, // (T*, std::ostream*, const std::filesystem::path*, StreamType)
	};
	struct wrapper_input {
		wrapper_op op;
		StreamType stype;
		void* data; // either T* or SerializeWrapper* for Construct
		const std::filesystem::path* path;
	};
	using wrapper_fn = void(wrapper_input input);
	/**
	 * Deserialize the data.
	 */
	void load(std::istream& in, const std::filesystem::path& fname, StreamType type);
	/**
	 * Serialize the data
	 */
	void save(std::ostream& out, const std::filesystem::path& fname, StreamType type);

	std::type_index type() const noexcept { return m_type; }
	void* data() noexcept { return m_data.get(); }
	const void* data() const noexcept { return m_data.get(); }

	template <inx::NonConstPlain T>
	T& as()
	{
		// if data() == nullptr, m_type will throw
		if (m_type != typeid(T)) {
			throw std::bad_cast();
		}
		return *static_cast<T*>(m_data.get());
	}
	template <inx::ConstPlain T>
	T& as() const
	{
		using Treal = std::remove_const_t<T>;
		// if data() == nullptr, m_type will throw
		if (m_type != typeid(Treal)) {
			throw std::bad_cast();
		}
		return *const_cast<const T*>(static_cast<Treal*>(m_data.get()));
	}

protected:
	std::type_index m_type;
	inx::util::any_ptr m_data;
	wrapper_fn* m_operators;
};

template <typename T, SerMode OpenMode>
class SerializeWrap : public SerializeWrapper
{
public:
	static void wrapper_operator(wrapper_input input) {
		switch (input.op) {
		case wrapper_op::Construct:
			*static_cast<inx::util::any_ptr*>(input.data) = std::make_unique<T>();
			break;
		case wrapper_op::Load: {
			std::ios::openmode mode;
			if constexpr (concepts::has_ser_load_mode<T>) {
				mode = T::ser_load_mode();
			} else if constexpr (concepts::has_ser_binary<T>) {
				mode = std::ios::in | (T::ser_binary() ? std::ios::binary : 0);
			} else {
				mode = std::ios::in;
			}
			assert(input.data != nullptr && input.path != nullptr);
			T* data = static_cast<T*>( static_cast<inx::util::any_ptr*>(input.data)->get() );
			if (data == nullptr)
				throw std::runtime_error("Serialize Load/Save requires Construct first.");
			serialize_load(*data, *input.path, input.stype, mode);
			break;
		}
		case wrapper_op::Save: {
			std::ios::openmode mode;
			if constexpr (concepts::has_ser_save_mode<T>) {
				mode = T::ser_save_mode();
			} else if constexpr (concepts::has_ser_binary<T>) {
				mode = std::ios::out | std::ios::trunc | (T::ser_binary() ? std::ios::binary : 0);
			} else {
				mode = std::ios::out | std::ios::trunc;
			}
			assert(input.data != nullptr && input.path != nullptr);
			T* data = static_cast<T*>( static_cast<inx::util::any_ptr*>(input.data)->get() );
			if (data == nullptr)
				throw std::runtime_error("Serialize Load/Save requires Construct first.");
			serialize_save(*data, *input.path, input.stype, mode);
			break;
		}
		default:
			assert(false);
		}
	};
};

} // namespace inx::flow::data

#endif // INXFLOW_DATA_SERIALIZE_HPP
