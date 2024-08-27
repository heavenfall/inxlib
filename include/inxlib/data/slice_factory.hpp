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

#ifndef INXLIB_DATA_SLICE_FACTORY_HPP
#define INXLIB_DATA_SLICE_FACTORY_HPP

#include <inxlib/inx.hpp>
#include <inxlib/util/bits.hpp>
#include <memory_resource>

namespace inx::data {

namespace details {

template <size_t SliceSize, size_t SlabSize, size_t SlabMinLevel = 0>
class SliceFactoryImpl {
public:
	static_assert(SlabSize >= SliceSize,
	              "SlabSize must be at least as large as SliceSize");
	using self = SliceFactoryImpl<SliceSize, SlabSize>;
	using value_type = std::byte;
	using pointer = value_type*;
	constexpr static size_t align_size() noexcept {
		return sizeof(std::max_align_t);
	}
	constexpr static size_t padding1() noexcept {
		return align_size() >= sizeof(void*)
		           ? align_size()
		           : sizeof(void*) +
		                 (align_size() - sizeof(void*) % align_size()) %
		                     align_size();
	}
	constexpr static size_t padding2() noexcept {
		return padding1() +
		       (align_size() >= sizeof(size_t)
		            ? align_size()
		            : sizeof(size_t) +
		                  (align_size() - sizeof(size_t) % align_size()) %
		                      align_size());
	}
	constexpr static size_t slice_size() noexcept { return SliceSize; }
	constexpr static size_t slab_size() noexcept {
		return std::max(
		    (SlabSize + align_size() - 1) / align_size() * align_size(),
		    2 * std::max(align_size(), sizeof(void*)));
	}
	constexpr static size_t init_slab_count() noexcept {
		return slab_size() / slice_size();
	}
	constexpr static size_t slab_min_level() noexcept {
		return std::max(
		    static_cast<size_t>(
		        slice_size() < padding1()
		            ? util::clz_index((slice_size() + padding1() - 1) /
		                              padding1())
		            : 0),
		    SlabMinLevel);
	}
	constexpr static size_t slab_max_level() noexcept {
		return util::clz_index(init_slab_count());
	}

	struct SlabCtrl {
		pointer reuse;
		pointer slab;
		SlabCtrl() noexcept : reuse(nullptr), slab(nullptr) {}
	};
	struct Data {
		std::pmr::memory_resource* mr;
		std::array<SlabCtrl, 32> slabs;
		pointer slabReuse;

		Data() noexcept : slabReuse(nullptr) {
			mr = std::pmr::get_default_resource();
		}
		Data(std::pmr::memory_resource& l_mr) noexcept
		    : mr(&l_mr), slabReuse(nullptr) {}
	};

	SliceFactoryImpl() = default;
	SliceFactoryImpl(std::pmr::memory_resource& l_mr) noexcept : m_data(l_mr) {}
	SliceFactoryImpl(SliceFactoryImpl&) = delete;
	~SliceFactoryImpl() { free(); }

	void free() {
		free_range(slab_min_level(), m_data.slabs.size());
		free_reuse();
	}

	// releases all memory, reuses blocks
	void reset() {
		pointer reuse = m_data.slabReuse;
		for (size_t i = slab_min_level(); i <= slab_max_level(); ++i) {
			SlabCtrl& slab = m_data.slabs[i];
			if (pointer at = slab.slab; at != nullptr) {
				slab.reuse = nullptr;
				slab.slab = nullptr;
				get_slab_pointer2_at(at) = std::exchange(reuse, at);
			}
		}
		m_data.slabReuse = reuse;

		free_range(slab_max_level() + 1, m_data.slabs.size());
	}

	[[nodiscard]] void* allocate(size_t level) {
		if (level >= 32) throw std::logic_error("level must be within [0,32)");
		if constexpr (slab_min_level() > 0) {
			if (level < slab_min_level()) level = slab_min_level();
		}
		SlabCtrl& ctrl = m_data.slabs[level];
		pointer data;
		if (ctrl.reuse != nullptr) {
			data = std::exchange(ctrl.reuse, get_reuse_pointer_at(ctrl.reuse));
		} else if (level <= slab_max_level()) {
			size_t chunk = slice_size() << level;
			if (ctrl.slab == nullptr ||
			    get_slab_pos_at(ctrl.slab) + chunk > slab_size()) {
				pointer newSlab = allocate_slab(level);
				get_slab_pos_at(newSlab) = 0;
				get_slab_pointer_at(newSlab) =
				    std::exchange(ctrl.slab, newSlab);
			}
			auto& p = get_slab_pos_at(ctrl.slab);
			data = ctrl.slab + p;
			p += chunk;
		} else {
			data = allocate_slab(level);
			get_slab_pointer_at(data) = std::exchange(ctrl.slab, data);
		}
		return data;
	}
	void deallocate(void* d_ptr, size_t level) {
		pointer ptr = static_cast<pointer>(d_ptr);
		if (level >= 32) throw std::logic_error("level must be within [0,32)");
		if constexpr (slab_min_level() > 0) {
			if (level < slab_min_level()) level = slab_min_level();
		}
		if (level <= slab_max_level()) {
			get_reuse_pointer_at(ptr) =
			    std::exchange(m_data.slabs[level].reuse, ptr);
		} else {
			get_slab_pointer_at(ptr) =
			    std::exchange(m_data.slabs[level].reuse, ptr);
		}
	}

protected:
	void free_range(size_t i, size_t ie) {
		auto* mr = m_data.mr;
		for (; i < ie; ++i) {
			SlabCtrl& slab = m_data.slabs[i];
			if (pointer at = slab.slab; at != nullptr) {
				slab.reuse = nullptr;
				slab.slab = nullptr;
				size_t size = get_slab_size_padded(i);
				while (true) {
					pointer nx = get_slab_pointer_at(at);
					mr->deallocate(at - padding2(), size, align_size());
					if (nx == nullptr) break;
					at = nx;
				}
			}
		}
	}
	void free_reuse() {
		constexpr size_t size = get_slab_size_padded(slab_min_level());
		auto* mr = m_data.mr;
		if (pointer at = m_data.slabReuse; at != nullptr) {
			while (true) {
				pointer nx = get_slab_pointer2_at(at);
				mr->deallocate(at - padding2(), size, align_size());
				if (nx == nullptr) break;
				at = nx;
			}
		}
	}
	constexpr static pointer& get_reuse_pointer_at(pointer slab) noexcept {
		return *reinterpret_cast<pointer*>(slab);
	}
	constexpr static pointer& get_slab_pointer_at(pointer slab) noexcept {
		return *reinterpret_cast<pointer*>(slab - padding1());
	}
	constexpr static pointer& get_slab_pointer2_at(pointer slab) noexcept {
		return *reinterpret_cast<pointer*>(slab - padding2());
	}
	constexpr static size_t& get_slab_pos_at(pointer slab) noexcept {
		return *reinterpret_cast<size_t*>(slab - padding2());
	}
	// constexpr static size_t get_slab_size(size_t index) noexcept
	// {
	// 	return index <= slab_max_level() ? slab_size() : ( (slab_size() <<
	// index-slab_max_level()) );
	// }
	constexpr static size_t get_slab_size_padded(size_t index) noexcept {
		return index <= slab_max_level()
		           ? (slab_size() + padding2())
		           : ((slice_size() << index) + padding2());
	}

	pointer allocate_slab(size_t level) {
		assert(level >= slab_min_level() && level < 32);
		if (level <= slab_max_level()) {
			if (pointer slab = m_data.slabReuse;
			    slab != nullptr) {  // reuse old slab
				if (pointer& slab2 = get_slab_pointer_at(slab);
				    slab2 != nullptr) {  // next slab on same level
					slab = slab2;
					slab2 = get_slab_pointer_at(slab2);
					return slab;
				}
				m_data.slabReuse = get_slab_pointer2_at(slab);
				return slab;
			}
		}
		pointer slab = static_cast<pointer>(
		    m_data.mr->allocate(get_slab_size_padded(level), align_size()));
		slab += padding2();
		return slab;
	}

	Data m_data;
};

}  // namespace details

template <typename ValueType, size_t SlabSize, size_t SlabMinLevel = 0>
class SliceFactory : public details::SliceFactoryImpl<sizeof(ValueType),
                                                      SlabSize, SlabMinLevel> {
public:
	using self = SliceFactory<ValueType, SlabSize, SlabMinLevel>;
	using super =
	    details::SliceFactoryImpl<sizeof(ValueType), SlabSize, SlabMinLevel>;
	using super::super;
};

template <typename ValueType, size_t N, size_t SlabMinLevel = 0>
using SliceFactoryN =
    SliceFactory<ValueType, N * sizeof(ValueType), SlabMinLevel>;

}  // namespace inx::data

#endif  // INXLIB_DATA_SLICE_FACTORY_HPP
