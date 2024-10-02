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

#include <concepts>
#include <fstream>
#include <inxlib/io/null.hpp>
#include <inxlib/types.hpp>
#include <inxlib/util/functions.hpp>
#include <iostream>
#include <memory_resource>
#include <typeindex>

#include "types.hpp"

namespace inx::flow {

namespace data {

namespace details {
template <concepts::ser_load T>
bool
serialize_load_path_stream(T& data,
                           const std::filesystem::path& path,
                           std::istream& in,
                           StreamType stype)
{
	if constexpr (concepts::ser_load_full_type<T>) {
		data.load(path, in, stype);
		return true;
	} else if constexpr (concepts::ser_load_full<T>) {
		data.load(path, in);
		return true;
	} else if constexpr (concepts::ser_load_stream_type<T>) {
		data.load(in, stype);
		return true;
	} else if constexpr (concepts::ser_load_stream<T>) {
		data.load(in);
		return true;
	}
	return false;
}
template <concepts::ser_load T>
bool
serialize_load_stream(T& data, std::istream& in, StreamType stype)
{
	if constexpr (concepts::ser_load_stream_type<T>) {
		data.load(in, stype);
		return true;
	} else if constexpr (concepts::ser_load_stream<T>) {
		data.load(in);
		return true;
	} else if constexpr (concepts::ser_load_full_type<T>) {
		data.load({}, in, stype);
		return true;
	} else if constexpr (concepts::ser_load_full<T>) {
		data.load({}, in);
		return true;
	}
	return false;
}
template <concepts::ser_load T>
bool
serialize_load_file(T& data,
                    const std::filesystem::path& path,
                    std::ios_base::openmode mode)
{
	if constexpr (concepts::ser_load_filename<T>) {
		return data.load(path);
	} else {
		std::ifstream in(path, mode);
		if (!in)
			return false;
		return serialize_load_path_stream(data, path, in, StreamType::File);
	}
}

template <concepts::ser_save T>
bool
serialize_save_path_stream(T& data,
                           const std::filesystem::path& path,
                           std::ostream& out,
                           StreamType stype)
{
	if constexpr (concepts::ser_save_full_type<T>) {
		data.save(path, out, stype);
		return true;
	} else if constexpr (concepts::ser_save_full<T>) {
		data.save(path, out);
		return true;
	} else if constexpr (concepts::ser_save_stream_type<T>) {
		data.save(out, stype);
		return true;
	} else if constexpr (concepts::ser_save_stream<T>) {
		data.save(out);
		return true;
	}
	return false;
}
template <concepts::ser_save T>
bool
serialize_save_stream(T& data, std::istream& in, StreamType stype)
{
	if constexpr (concepts::ser_save_stream_type<T>) {
		data.save(in, stype);
		return true;
	} else if constexpr (concepts::ser_save_stream<T>) {
		data.save(in);
		return true;
	} else if constexpr (concepts::ser_save_full_type<T>) {
		data.save({}, in, stype);
		return true;
	} else if constexpr (concepts::ser_save_full<T>) {
		data.save({}, in);
		return true;
	}
	return false;
}
template <concepts::ser_save T>
bool
serialize_save_file(T& data,
                    const std::filesystem::path& path,
                    std::ios_base::openmode mode)
{
	if constexpr (concepts::ser_save_filename<T>) {
		return data.save(path);
	} else {
		std::ofstream in(path, mode);
		if (!in)
			return false;
		return serialize_save_path_stream(data, path, in, StreamType::File);
	}
}
} // namespace details

template <concepts::ser_load T>
bool
serialize_load(T& data,
               std::istream& in,
               const std::filesystem::path& path,
               StreamType stype,
               std::ios_base::openmode mode = std::ios::in)
{
	switch (stype) {
	case StreamType::File:
		return details::serialize_load_file(data, path, mode);
	case StreamType::Stream:
		if (path.empty()) {
			return details::serialize_load_stream(data, in, StreamType::Stream);
		} else {
			return details::serialize_load_path_stream(
			  data, path, StreamType::Stream);
		}
	case StreamType::StdIn:
		return details::serialize_load_stream(
		  data, std::cin, StreamType::StdIn);
	case StreamType::DevNull: {
		inx::io::null_istream nulls;
		return details::serialize_load_stream(data, nulls, StreamType::DevNull);
	}
	case StreamType::StdOut:
	default:
		return false;
	}
}

template <concepts::ser_save T>
bool
serialize_save(T& data,
               std::ostream& out,
               const std::filesystem::path& path,
               StreamType stype,
               std::ios_base::openmode mode = std::ios::out | std::ios::trunc)
{
	switch (stype) {
	case StreamType::File:
		return details::serialize_save_file(data, path, mode);
	case StreamType::Stream:
		if (path.empty()) {
			return details::serialize_save_stream(
			  data, out, StreamType::Stream);
		} else {
			return details::serialize_save_stream(
			  data, path, StreamType::Stream);
		}
	case StreamType::StdOut:
		return details::serialize_save_stream(
		  data, std::cin, StreamType::StdIn);
	case StreamType::DevNull: {
		inx::io::null_istream nulls;
		return details::serialize_save_stream(data, nulls, StreamType::DevNull);
	}
	case StreamType::StdIn:
	default:
		return false;
	}
}

//
// SerializeWrapper
//

namespace concepts {
template <typename T>
concept has_ser_binary = requires { T::ser_binary(); };
template <typename T>
concept has_ser_load_mode = requires { T::concepts::ser_load_mode(); };
template <typename T>
concept has_ser_save_mode = requires { T::concepts::ser_save_mode(); };
} // namespace concepts

class Serialize;

} // namespace data

using serialize = std::shared_ptr<data::Serialize>;

namespace data {

namespace details {
template <typename T, auto LoadFunc>
struct SerializeLoader
{
	T* data;
	void load(std::istream& in,
	          const std::filesystem::path& filename,
	          StreamType type)
	    requires requires(T& t) { LoadFunc(t, in, filename, type); }
	{
		LoadFunc(*data, in, filename, type);
	}
	void load(std::istream& in, const std::filesystem::path& filename)
	    requires requires(T& t) { LoadFunc(t, in, filename); }
	{
		LoadFunc(*data, in, filename);
	}
	void load(std::istream& in, StreamType type)
	    requires requires(T& t) { LoadFunc(t, in, type); }
	{
		LoadFunc(*data, in, type);
	}
	void load(std::istream& in)
	    requires requires(T& t) { LoadFunc(t, in); }
	{
		LoadFunc(*data, in);
	}
	void load(const std::filesystem::path& filename)
	    requires requires(T& t) { LoadFunc(t, filename); }
	{
		LoadFunc(*data, filename);
	}
};

template <typename T, auto SaveFunc>
struct SerializeSaver
{
	T* data;
	void save(std::ostream& in,
	          const std::filesystem::path& filename,
	          StreamType type)
	    requires requires(T& t) { SaveFunc(t, in, filename, type); }
	{
		SaveFunc(*data, in, filename, type);
	}
	void save(std::ostream& in, const std::filesystem::path& filename)
	    requires requires(T& t) { SaveFunc(t, in, filename); }
	{
		SaveFunc(*data, in, filename);
	}
	void save(std::ostream& in, StreamType type)
	    requires requires(T& t) { SaveFunc(t, in, type); }
	{
		SaveFunc(*data, in, type);
	}
	void save(std::ostream& in)
	    requires requires(T& t) { SaveFunc(t, in); }
	{
		SaveFunc(*data, in);
	}
	void save(const std::filesystem::path& filename)
	    requires requires(T& t) { SaveFunc(t, filename); }
	{
		SaveFunc(*data, filename);
	}
};
} // namespace details

class Serialize : public std::enable_shared_from_this<Serialize>
{
public:
	struct wrapper_input
	{
		wrapper_op op;
		wrapper_op support;
		StreamType stype;
		void* stream; // either any_ptr* or stream*
		void* data;
		const std::filesystem::path* path;
	};
	using wrapper_fn = bool(wrapper_input input);
	using serialize_construct =
	  serialize(const std::pmr::polymorphic_allocator<>&);

protected:
	Serialize(std::type_index type, wrapper_fn* fn, serialize_construct* dup);

public:
	Serialize() = delete;
	Serialize(const Serialize& other);
	Serialize(Serialize&& other);
	Serialize(const Serialize& other, bool copy);

	Serialize& operator=(const Serialize& other);
	Serialize& operator=(Serialize&& other);

	serialize construct_new(
	  const std::pmr::polymorphic_allocator<>& alloc) const;

protected:
	void copy_(const void* other);
	void move_(void* other);

public:
	void clear();

	template <typename T>
	void copy(const T& other)
	{
		if (m_type != typeid(T))
			throw std::logic_error("type mismatch");
		copy_(&other);
	}
	template <typename T>
	void move(T&& other)
	{
		if (m_type != typeid(T))
			throw std::logic_error("type mismatch");
		move_(&other);
	}

	bool supported(wrapper_op op) const;

	/**
	 * Deserialize the data.
	 */
	void load(std::istream& in,
	          const std::filesystem::path& fname,
	          StreamType type);
	/**
	 * Serialize the data
	 */
	void save(std::ostream& out,
	          const std::filesystem::path& fname,
	          StreamType type);

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
	serialize_construct* m_duplicate;
};

namespace concepts {
template <typename T>
concept serializable = std::derived_from<T, Serialize>;
}

/**
 * Wrapper class for type T to serialize.
 * OpenMode determines if file should be opened in text, binary or determined by
 * T (auto). LoadFunc overrides the load function, nullptr will use T::load
 * SaveFunc overrides the save function, nullptr will use T::save
 */
template <typename T,
          SerMode OpenMode = SerMode::Auto,
          auto LoadFunc = nullptr,
          auto SaveFunc = nullptr>
class SerializeWrap : public Serialize
{
public:
	static constexpr bool load_func_null =
	  std::is_null_pointer_v<decltype(LoadFunc)>;
	static constexpr bool save_func_null =
	  std::is_null_pointer_v<decltype(LoadFunc)>;

protected:
	static bool wrapper_operator_(wrapper_input input)
	{
		switch (input.op) {
		case wrapper_op::Support:
			switch (input.support) {
			case wrapper_op::Construct:
				return true;
			case wrapper_op::Copy:
				return std::is_copy_constructible_v<T> ||
				       std::is_copy_assignable_v<T>;
			case wrapper_op::Move:
				return std::is_move_constructible_v<T> ||
				       std::is_move_assignable_v<T>;
			case wrapper_op::Load:
				return !load_func_null || concepts::ser_load<T>;
			case wrapper_op::Save:
				return !save_func_null || concepts::ser_save<T>;
			default:
				return false;
			}
		case wrapper_op::Construct:
			*static_cast<inx::util::any_ptr*>(input.stream) =
			  std::make_unique<T>();
			return true;
		case wrapper_op::Copy:
			if constexpr (std::is_copy_assignable_v<T>) {
				if constexpr (!std::is_copy_constructible_v<T>) {
					// construct then copy
					auto& ptr = *static_cast<inx::util::any_ptr*>(input.stream);
					if (!ptr) {
						ptr = std::make_unique<T>();
					}
					*static_cast<T*>(ptr.get()) = *static_cast<const T*>(
					  const_cast<const void*>(input.data));
					return true;
				} else { // try to copy
					if (auto* g =
					      static_cast<inx::util::any_ptr*>(input.stream)->get();
					    g != nullptr) {
						*static_cast<T*>(g) = *static_cast<const T*>(
						  const_cast<const void*>(input.data));
						return true;
					}
				}
			}
			if constexpr (std::is_copy_constructible_v<T>) { // try to copy
				                                             // construct
				*static_cast<inx::util::any_ptr*>(input.stream) =
				  std::make_unique<T>(*static_cast<const T*>(
				    const_cast<const void*>(input.data)));
				return true;
			}
			return false;
		case wrapper_op::Move:
			if constexpr (std::is_move_assignable_v<T>) {
				if constexpr (!std::is_move_constructible_v<T>) { // construct
					                                              // then move
					auto& ptr = *static_cast<inx::util::any_ptr*>(input.stream);
					if (!ptr) {
						ptr = std::make_unique<T>();
					}
					*static_cast<T*>(ptr.get()) =
					  std::move(*static_cast<T*>(input.data));
					return true;
				} else { // try to move
					if (auto* g =
					      static_cast<inx::util::any_ptr*>(input.stream)->get();
					    g != nullptr) {
						*static_cast<T*>(g) =
						  std::move(*static_cast<T*>(input.data));
						return true;
					}
				}
			}
			if constexpr (std::is_move_constructible_v<T>) { // try to move
				                                             // construct
				*static_cast<inx::util::any_ptr*>(input.stream) =
				  std::make_unique<T>(std::move(*static_cast<T*>(input.data)));
				return true;
			}
			return false;
		case wrapper_op::Load: {
			std::ios::openmode mode;
			if constexpr (concepts::has_ser_load_mode<T>) {
				mode = T::concepts::ser_load_mode();
			} else if constexpr (concepts::has_ser_binary<T>) {
				mode = std::ios::in | (T::ser_binary() ? std::ios::binary : 0);
			} else {
				mode = std::ios::in;
			}
			assert(input.data != nullptr && input.path != nullptr);
			T* data = static_cast<T*>(
			  static_cast<inx::util::any_ptr*>(input.data)->get());
			if (data == nullptr)
				throw std::runtime_error(
				  "Serialize Load/Save requires Construct first.");
			if constexpr (!load_func_null) {
				details::SerializeLoader<T, LoadFunc> loader{data};
				serialize_load(loader, *input.path, input.stype, mode);
			} else {
				serialize_load(*data, *input.path, input.stype, mode);
			}
			break;
		}
		case wrapper_op::Save: {
			std::ios::openmode mode;
			if constexpr (concepts::has_ser_save_mode<T>) {
				mode = T::concepts::ser_save_mode();
			} else if constexpr (concepts::has_ser_binary<T>) {
				mode = std::ios::out | std::ios::trunc |
				       (T::ser_binary() ? std::ios::binary : 0);
			} else {
				mode = std::ios::out | std::ios::trunc;
			}
			assert(input.data != nullptr && input.path != nullptr);
			T* data = static_cast<T*>(
			  static_cast<inx::util::any_ptr*>(input.data)->get());
			if (data == nullptr)
				throw std::runtime_error(
				  "Serialize Load/Save requires Construct first.");
			if constexpr (!save_func_null) {
				details::SerializeSaver<T, SaveFunc> saver{data};
				serialize_save(saver, *input.path, input.stype, mode);
			} else {
				serialize_save(*data, *input.path, input.stype, mode);
			}
			break;
		}
		default:
			assert(false);
		}
	};

public:
	static serialize construct(const std::pmr::polymorphic_allocator<>* alloc)
	{
		serialize obj;
		if (alloc != nullptr) {
			obj = std::allocate_shared<SerializeWrap>(
			  *alloc,
			  typeid(SerializeWrap),
			  &SerializeWrap::wrapper_operator_,
			  &SerializeWrap::construct);
		} else {
			obj =
			  std::make_shared<SerializeWrap>(typeid(SerializeWrap),
			                                  &SerializeWrap::wrapper_operator_,
			                                  &SerializeWrap::construct);
		}
		return obj;
	}
};

} // namespace data
} // namespace inx::flow

#endif // INXFLOW_DATA_SERIALIZE_HPP
