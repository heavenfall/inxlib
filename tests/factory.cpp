#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <inxlib/memory/factory.hpp>
#include <inxlib/memory/source_factory.hpp>
#include <inxlib/memory/factory_pointer.hpp>
#include <inxlib/memory/single_factory.hpp>
#include <inxlib/memory/slab_factory.hpp>
#include <inxlib/memory/block_factory.hpp>
#include <inxlib/memory/bump_factory.hpp>
#include <inxlib/memory/indexed_block_factory.hpp>

#include <string_view>
#include <vector>
#include <set>

using namespace std::string_view_literals;

namespace inx::memory {

template <typename T>
struct memblock {
	T a;
	T b;
};

TEST_CASE( "Basic factory check", "[factory]" ) {
	using memb = memblock<int32_t>;
	using memc = memblock<double>;

	SECTION( "concept check" ) {
		CHECK( ByteFactory<malloc_factory> );
		CHECK( ByteFactory<release_adaptor<malloc_factory>> );
		CHECK( SingleFactory<single_factory<malloc_factory, 4>> );
		CHECK( VoidFactory<void_factory> );
		CHECK_FALSE( VoidFactory<malloc_factory> );
		CHECK( ReleaseFactory<release_adaptor<malloc_factory>> );
		CHECK_FALSE( ReclaimFactory<release_adaptor<malloc_factory>> );
		CHECK( ByteFactory< factory_pointer< release_adaptor<malloc_factory>> > );
		CHECK( SingleFactory< factory_pointer<single_factory<malloc_factory, 4>> > );
	}

	SECTION( "single factory" ) {
		single_factory_type<malloc_factory, memb> factory;
		static_assert(SingleFactory<decltype(factory)>, "single_factory must be SingleFactory");
		REQUIRE( factory.element_size() == sizeof(memb) );
		REQUIRE( factory.alignment() == alignof(memb) );
		auto p1 = reinterpret_cast<memb*>(factory.create());
		p1->a = 342;
		p1->b = 4224;
		auto p2 = reinterpret_cast<memb*>(factory.create());
		p2->a = 3242;
		factory.destroy(reinterpret_cast<std::byte*>(p2));
		auto p3 = reinterpret_cast<memb*>(factory.create());
		auto p4 = reinterpret_cast<memb*>(factory.create());
		p3->a = p4->b = 324222;
		factory.destroy(reinterpret_cast<std::byte*>(p4));
		factory.destroy(reinterpret_cast<std::byte*>(p1));
		factory.destroy(reinterpret_cast<std::byte*>(p3));
	}

	SECTION( "reuse factory" ) {
		single_factory_type<malloc_factory, memb> factory;
		static_assert(SingleFactory<decltype(factory)>, "single_factory must be SingleFactory");
		REQUIRE( factory.element_size() == sizeof(memb) );
		std::set<memb*> alloc;
		REQUIRE( factory.alignment() == alignof(memb) );
		auto p1 = reinterpret_cast<memb*>(factory.create());
		alloc.insert(p1);
		p1->a = 342;
		p1->b = 4224;
		auto p2 = reinterpret_cast<memb*>(factory.create());
		alloc.insert(p2);
		p2->a = 3242;
		factory.destroy(reinterpret_cast<std::byte*>(p2));
		auto p3 = reinterpret_cast<memb*>(factory.create());
		CHECK( alloc.contains(p3) );
		auto p4 = reinterpret_cast<memb*>(factory.create());
		p3->a = p4->b = 324222;
		factory.destroy(reinterpret_cast<std::byte*>(p4));
		factory.destroy(reinterpret_cast<std::byte*>(p1));
		factory.destroy(reinterpret_cast<std::byte*>(p3));
	}

	SECTION( "area factory" ) {
		using base_bump_factory = bump_factory<slab_factory<malloc_factory, 0>>;
		static_assert(ArrayFactory<base_bump_factory>, "bump_factory must be ArrayFactory");
		base_bump_factory ba;
		slab_factory< factory_pointer<base_bump_factory>, 128, slab_memory_type<memb> > a1;
		slab_factory< factory_pointer<base_bump_factory>, 256, slab_memory_type<memc> > a2;

		REQUIRE( ba.setup(slab_factory_params(1024 * 128)) );
		REQUIRE( a1.setup(ba) );
		REQUIRE( a2.setup(ba) );

		for (int i = 0; i < 1024; ++i) {
			auto* p1 [[maybe_unused]] = a1.create();
			auto* p2 [[maybe_unused]] = a2.create();
		}
	}

	SECTION( "bump factory" ) {
		using bump_factory = bump_factory<slab_factory<malloc_factory, 256>>;
		static_assert(ArrayFactory<bump_factory>, "bump_factory must be ArrayFactory");
		single_factory_type<bump_factory, memb> ba;
		single_factory<bump_factory, 0, 0> bb;
		REQUIRE( ba.setup() );
		REQUIRE( ba.element_size() == sizeof(memb) );
		REQUIRE( ba.alignment() >= alignof(memb) );
		REQUIRE( bb.setup(dynamic_factory_params(sizeof(int32_t), alignof(int32_t))) );
		REQUIRE( bb.element_size() == sizeof(int32_t) );
		REQUIRE( bb.alignment() >= alignof(int32_t) );
		std::vector<memb*> ref;
		std::vector<int32_t*> refb;
		constexpr int32_t TOTAL = 1024 * 16;

		auto* big = reinterpret_cast<int32_t*>( ba.upstream().allocate(TOTAL * sizeof(int32_t)) );
		for (int32_t i = 0; i < TOTAL; ++i) {
			big[i] = -i;
		}
		for (int32_t i = 0; i < TOTAL; ++i) {
			auto* ptr = reinterpret_cast<memb*>( ba.create() );
			ptr->a = i;
			ptr->b = i*i;
			ref.push_back(ptr);
			int32_t* ptrb = reinterpret_cast<int32_t*>( bb.create() );
			*ptrb = i + i/2;
			refb.push_back(ptrb);
		}
		for (int32_t i = 0; i < TOTAL; ++i) {
			REQUIRE(big[i] == -i);
			auto* ptr = ref[i];
			REQUIRE(ptr->a == i);
			REQUIRE(ptr->b == i*i);
			auto* ptrb = refb[i];
			REQUIRE(*ptrb == i + i/2);
		}
	}

	SECTION( "block factory" ) {
		using area_fact = slab_factory<malloc_factory, 1024>;
		using block_fact_static = block_factory_type<memc, factory_pointer<area_fact>>;
		static_assert(SingleFactory<block_fact_static>, "block_factory must be SingleFactory");
		area_fact pool;
		REQUIRE( pool.setup() );
		block_fact_static ba;
		REQUIRE( ba.setup(pool) );
		std::vector<memc*> ref;
		constexpr int32_t TOTAL = 1024;
		std::set<memc*> alloc;

		for (int32_t i = 0; i < TOTAL; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.create());
			v->a = i;
			v->b = i*i;
			REQUIRE_FALSE( alloc.contains(v) );
			alloc.insert(v);
		}
		ba.reclaim();
		int32_t dup_count = 0;
		for (int32_t i = 0; i < TOTAL; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.create());
			if (alloc.contains(v))
				dup_count += 1;
		}
		// requires some reuse of blocks
		REQUIRE( dup_count > 0 );
	}

	SECTION( "buffer factory" ) {
		buffer_factory<1024> factory_void;
		factory_void.setup();
		int valid = 0, invalid = 0;
		for (int i = 0; i < 16; i++) {
			auto* p = factory_void.allocate(128);
			if (p)
				valid += 1;
			else
				invalid += 1;
		}
		CHECK( valid != 0 );
		CHECK( valid <= 8 );
		CHECK( invalid >= 8 );
		factory_void.release();
		valid = 0, invalid = 0;
		for (int i = 0; i < 16; i++) {
			auto* p = factory_void.allocate(128);
			if (p)
				valid += 1;
			else
				invalid += 1;
		}
		CHECK( valid != 0 );
		CHECK( valid <= 8 );
		CHECK( invalid >= 8 );

		buffer_factory<1024, malloc_factory> factory_malloc;
		factory_malloc.setup();
		valid = 0, invalid = 0;
		for (int i = 0; i < 64; i++) {
			auto* p = factory_malloc.allocate(128);
			if (p)
				valid += 1;
			else
				invalid += 1;
		}
		REQUIRE( invalid == 0 );
		factory_malloc.reclaim();
		valid = 0, invalid = 0;
		for (int i = 0; i < 64; i++) {
			auto* p = factory_malloc.allocate(128);
			if (p)
				valid += 1;
			else
				invalid += 1;
		}
		REQUIRE( invalid == 0 );

		using bfactory = bump_factory< slab_factory<malloc_factory, 4096> >;
		buffer_factory<1024, bfactory> factory_bump;
		factory_bump.setup();
		valid = 0, invalid = 0;
		for (int i = 0; i < 1024; i++) {
			auto* p = factory_bump.allocate(128);
			if (p)
				valid += 1;
			else
				invalid += 1;
		}
		REQUIRE( invalid == 0 );
		factory_bump.upstream().reclaim();
		factory_bump.release(false);
		valid = 0, invalid = 0;
		for (int i = 0; i < 1024; i++) {
			auto* p = factory_bump.allocate(128);
			if (p)
				valid += 1;
			else
				invalid += 1;
		}
		REQUIRE( invalid == 0 );
	}

	SECTION( "indexed block factory" ) {
		using area_fact = slab_factory<malloc_factory, 1024>;
		using indexed_fact = indexed_block_factory_type<memc, factory_pointer<area_fact>>;
		static_assert(SingleFactory<indexed_fact>, "block_factory must be SingleFactory");
		area_fact pool;
		REQUIRE( pool.setup() );
		indexed_fact ba;
		REQUIRE( ba.setup(pool) );
		std::vector<memc*> ref;
		constexpr int32_t TOTAL = 1024;
		std::set<memc*> alloc;

		// allocate inital 1024 elements one-at-a-time
		for (int32_t i = 0; i < TOTAL; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.create());
			v->a = i;
			v->b = i*i;
			REQUIRE_FALSE( alloc.contains(v) );
			alloc.insert(v);
			ref.push_back(v);
		}
		REQUIRE( ba.size() == TOTAL );
		// check previous allocations
		for (int32_t i = 0; i < TOTAL; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.get_if(i));
			REQUIRE( v == ref[i] );
			REQUIRE( v->a == i );
			REQUIRE( v->b == (i*i) );
		}
		constexpr int32_t TOTAL2 = 10 * TOTAL;
		ba.resize(TOTAL2);
		for (int32_t i = 0; i < TOTAL; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.get_if(i));
			REQUIRE( v == ref[i] );
		}
		for (int32_t i = TOTAL; i < TOTAL2; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.get_if(i));
			v->a = i;
			v->b = i*i;
			REQUIRE_FALSE( alloc.contains(v) );
			alloc.insert(v);
			ref.push_back(v);
		}
	}

	SECTION( "runtime indexed block factory" ) {
		using area_fact = slab_factory<malloc_factory, 1024>;
		using indexed_fact = indexed_block_factory<factory_pointer<area_fact>, malloc_factory>;
		static_assert(SingleFactory<indexed_fact>, "block_factory must be SingleFactory");
		area_fact pool;
		REQUIRE( pool.setup() );
		indexed_fact ba;
		REQUIRE( ba.setup(indexed_block_factory_params_type<memc>, std::tuple<>(), pool) );
		std::vector<memc*> ref;
		constexpr int32_t TOTAL = 1024;
		std::set<memc*> alloc;

		// allocate inital 1024 elements one-at-a-time
		for (int32_t i = 0; i < TOTAL; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.create());
			v->a = i;
			v->b = i*i;
			REQUIRE_FALSE( alloc.contains(v) );
			alloc.insert(v);
			ref.push_back(v);
		}
		REQUIRE( ba.size() == TOTAL );
		// check previous allocations
		for (int32_t i = 0; i < TOTAL; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.get_if(i));
			REQUIRE( v == ref[i] );
			REQUIRE( v->a == i );
			REQUIRE( v->b == (i*i) );
		}
		constexpr int32_t TOTAL2 = 10 * TOTAL;
		ba.resize(TOTAL2);
		for (int32_t i = 0; i < TOTAL; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.get_if(i));
			REQUIRE( v == ref[i] );
		}
		for (int32_t i = TOTAL; i < TOTAL2; ++i) {
			memc* v = reinterpret_cast<memc*>(ba.get_if(i));
			v->a = i;
			v->b = i*i;
			REQUIRE_FALSE( alloc.contains(v) );
			alloc.insert(v);
			ref.push_back(v);
		}
	}
}

}
