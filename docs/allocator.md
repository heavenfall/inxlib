# Memory Allocator

Headers in `inxlib/memory`, this provides a set of factory allocators using low-level allocators.
Features such as chaining allocations, choice of memory reuse and factories being able to reclaim
its allocations are part of this library.

## Core

Header `inblib/memory/factory.hpp` provides the base set of concepts for the allocator.

Enum `inx::memory::factory_traits` provides traits that identify behaviour of certain factories.
Key terms follow:
- Factory: provides memory allocation
- Upstream: an upstream factory that factory aquires allocations from
- Factory source: `FactorySource` is a factory that allocates memory, mainly malloc
- Factory ownership: `FactoryOwn` factory tracks the memory it allocates and can/will deallocate it
- Reuse: `FactoryReuse` factory can attempt to reuse deallocated memory from downstream/user calls
- No free: `FactoryNoFree` factory deallocator does nothing, relaise on upstream to deallocate
- Pointer: `FactoryPointer` factory is simply a pointer factory to another, specialised use

Factories are expected to provide:
- `typename value_type`: trivial type factory issues
- `typename pointer`: `value_type*`
- `typename size_type`: size type (just use `size_t`)
- must be default initializable
- must not be movable or copyable
- `setup(args...)`: initalize a factory for use
- `alignment()` allocation alignment, returns `size_type`, can be compile or runtime
- `element_size()` object size, returns `size_type`, can be compile or runtime
- `static traits()` `factory_traits` supported (use `|`), must be `uint32_t` and compile time known

There are two types of factories, detailed by concepts:
1. `SingleFactory` supports allocations of a single object of set size (compile or runtime) using `create` or `destroy` functions
2. `ArrayFactory` supports allocations of a variable number of memory consecutive objects of a set size using `allocate` or `deallocate` (optional)
These methods do not support constructors/destructors, nor is the memory gaurenteed to be initalised (zerod).

Concept `Factory` takes either `SingleFactory` or `ArrayFactory` classes.

Concept `ByteFactory` is an `ArrayFactory` designed to allocate elements as `std::byte`
with alignment of `max_align_t` (usually 16) like `malloc`.

Concept `AlignByteFactory` is a bytes factory that also supports `allocate` and `deallocate` with
runtime specified alignment value.

Class `void_factory` mimics a `ByteFactory` but does nothing, concept `VoidFactory` checks only for `void_factory`.

Concept `FreeArrayFactory` is an `ArrayFactory` that does not require knowing the allocation for deallocate function calls.

Concept `ReleaseFactory` is a `Factory` that supports `release()` and `release(bool)` function,
where `release(bool)` will releases all allocations to upstream if true, otherwise would logically reset the factory without deallocations.
A logical release is useful if an upstream resouce has released or reclaimed its memory, thus calling to upstream
or using the current allocators may be ill-formed.

Concept `ReclaimFactory` is a `ReleaseFactory` that supports `reclaim` function calls,
which invalidates all downstream allocations (memory from its create or allocate).
There is no mechenisms to automatically inform downstream that its memory has being reclaimed or released,
thus the user must call `release(false)` on all downstream manually (if supported) to prevent the program being ill-formed,
as the destructor will call `release(true)` which is ill-formed in this case.
If `reclaim` or `release` is used on an upstream factory, any future call to downstream `create` or `allocate` is ill-formed
until `release(false)` is called downstream, after which factory can be used as normal.

Function `upstream(Factory&)` return the `upstream` of provided factory, and `upstream<Factory>(Factory&)` returns a upstream factory
matching template class (i.e. chains until reaching template class).
While factories are chain inheritied, the inheritance is private and thus, calls to either `f.upstream()` or `upstream<?>(f)` must be made
to access upstream factories.

## Setup and Use

Factories are designed around chain-inheritance.
For example: `single_factory<malloc_factory, sizeof(int)>` has the top-level factory `single_factory`
inherit the `malloc_factory` as its upstream, resulting in `single_factory` serving int sized memory
blocks using the malloc upstream allocations (4-byte allocations).

As factories can be given run-time parameters, this is initalized with the `setup()` function call.
The end factory will require the user to call `setup(args...)`, with args being a set of arguments
used at runtime.
The methodolgy is that each factory will use 1 or 0 arguments (taken from the front), and pass the rest
to its upstream setup function.
It will return true if factory initalization succeeded, in which case the user can then use the allocator.
If any of the factories return false, then the allocator is not in a valid state to use.

# Factories

## Mermory Source Factories

The source factories are where memory origonates from.
These are all `ByteFactory`.

The primary method of allocating memory is through the `malloc_factory` in header `inxlib/memory/source_factory.hpp`.
This factory is a `BytesFactory`, and is non-owning, thus `deallocate` must be called for every
`allocate` else a memory leaks will occur.

The `memory_resource_factory` in header `inxlib/memory/source_factory.hpp` is a source factory that aquires memory
from an `std::pmr::memory_resource` pointer.
The `memory_resource` can reclaiming all its resources without this factory breaking, but further down the chain will
have to call a `release(false)`; the `memory_resource` going out of scope breaks this factory until `release(false)` is
called, and then `setup` to point at a new factory (must be done in this order).

The `memory_resource_factory` in header `inxlib/memory/source_factory.hpp` is given a C++ `std::pmr::memory_resource`.
Reuse of deallocated memory is dependent on the underlying `memory_resource`.

The `buffer_factory` in header `inxlib/memory/source_factory.hpp` is not strictly a source factory.
It is given a buffer size (in bytes), which allocations (both normal and aligned)
can be made,
and will return a memory address on that internal buffer (can be fully on the stack).
It has an `Upstream` factory, for use when that buffer is used up; if left to default,
allocation will fail.
Upstream allocation is also a block of specified size (or buffer size if 0).
This factory is owning, thus can release (frees all upsteam) or reclaim (reuse including upstream).

## Shaping Factories

The `single_factory` in header `inxlib/memory/single_factory.hpp` takes any `ArrayFactory` and provides allocations
as a `SingleFactory`.
This does not support a type, so user must specify in template parameters these values.
Use `single_factory_type` for this, though the upstream must be a `ByteFactory`.
If not using a `ByteFactory` upstream, the allocated size would be `Elems` times
the size of upstream `value_type`.

The other major shaping are the `AreaFactory` concept of factories, in `area_factory.hpp`.
These are detailed in their own section.

## Adaptors and Patterns

The adaptors in `inx/memory/factory_adaptor.hpp` are special factories that change the behaviour of a factory.
Unlike other factories, that inherit their `Upstream` as private, these inherit
public,
and are deisgned to add/mutate factory functions.
The major differences is a factory uses `Upstream` to source its own production,
whereas an adaptor simply adds to `Upstream` behaviour; therefore use of `Upstream` directly is discouraged,
use the adaptor version, as calling `Upstream` functions may break the adaptor.

A pattern is like an adaptor but is applied when inheriting a factory.
It is more for internal use, but will be listed here.

The `reuse_adaptor` is used on a `SingleFactory` without the `FactoryOwn` and `FactoryPointer` traits.
The `element_size()` must be big enough to hold a single pointer, otherwise `setup` will return false.
When destroy is called, instead of passing to upstream, it will be kept and used in a following `create`.
This is done by chaining, thus is `O(1)` and fast.
The `release` function is provided; if `Upstream` is a `ReleaseFactory`, this will call `Upstream` ones,
otherwise it will release up to upstream if `free_upstream` is true.

The `release_adaptor` takes a `ByteFactory Upstream` and adds a `release` function.
This will add two pointer size (typically 16 bytes) to each allocation for this feature.

The `overflow_pattern` gives a factory support for allocating memory in a way that is not standard for it.
This pattern introduces an overflow `ByteFactory`, in which memory allocations that do not fit normal
allocation will instead be allocated from an overflow.
For example, when allocated from a shared memory region but one allocation is as big as the whole region,
it will allocate from overflow instead of upstream.
Overflow can be `void_factory`, in which case it will search for the first upstream that is a `ByteFactory`
for the overflow allcoation.
The overflow factory can be access with `overflow()`, in the same mannor as presented above.

## Factory Pointer

The header `inxlib/memory/factory_chain.hpp` gives support methods for how factories chain together,
primarily a pointer wrapper to a factory.
The factory_pointer takes a `Factory`, is given a pointer and will make available the required
functions.
This is more a reference wrapper than a strict pointer, and adds support to share an upstream with multiple factories.
For example:
given some `ByteFactory Src`, for example a `bump_factory` that gives memory allocations out from a larger block,
the user would normally have to allocated `Src` sepretatly between two seperate `single_factory`, but by using
`factory_pointer<Src>`, a user can use the following code:

	Src src;
	src.setup();
	single_factory<factory_pointer<src>, sizeof(int)> fact_int;
	single_factory<factory_pointer<src>, sizeof(double)> fact_double;
	fact_int.setup(&src);
	fact_double.setup(&src);

This setup will have `fact_int` and `fact_double` both share `src` as their upstream.

## Area Factory

The `AreaFactory` concept is a pattern factory described in `inx/memory/area_factory.hpp`.
The class `area_factory` is a `SingleFactory`, it and its `pointer_factory` are the only `AreaFactory`.
The `area_factory` allocates static-sized blocked of RAM of template type `Area[]` from upstream.
The size is `AreaCount` number of `Area` types, plus 16 bytes of linkage for factory use.
It is intended to be used as a common type of other factories that stores in blocks.
Giving an `AreaCount` of 0 makes this factory a dynamic sized `AreaCount`, specified at runtime
through the setup function.

The `Area` defaults to `area_memory_bytes`, with element size of 1 and align of `max_align_t` (16).
Using `area_memory_type<T>` for specific type is also usable; although using the default is recommended.
Factories down the link can resahpe the type, thus `area_memory_bytes` works properly.

The `area_link_pattern` makes an `AreaFactory` be `FactoryOwn` and `FactoryReuse`.
This pattern manages the linkage space, though allows for `release` and `reclaim`.

The most basic use of of an `AreaFactory` is the `bump_factory`, a `ReclaimFactory`.
This uses the overflow pattern on the `AreaFactory`, and creates a `ByteFactory` that allocates
to `Area` chunks.
New allocations are put on chucks, making allocations mainly a pointer update.
Once an area is used up, a new area is created from upstream.
Large allocations (at least half size of area) that do not fit in a chuck allocate from overflow.

The `block_factory` in `inx/memory/block_factory.hpp` takes an `AreaFactory` upstream,
and similar to the `bump_factory`, splits those areas to its underlying type, except
this is `SingleFactory` that allocates with `Size` and `Align`.

# Example Usage

An advance usage is a program having allocations from a common memory pool.
This can be achived efficently from pointer allocations on a common `bump_factory`.

	// area factory with support of 4MB of allocation chunks
	using top_factory = bump_factory< area_factory<malloc_factory, 4 * 1024 * 1024> >;
	using top_pointer = factory_pointer<top_factory>;
	
	// new bump allocates that support 1008 byte blocks (16 reserved for pointer)
	using byte_1024 = bump_factory< area_factory<top_pointer, 1024-16> >;
	// a SingleFactory that allocates std::string_view, up to 512 per area block
	using sv_512 = block_factory_type<top_pointer, std::string_view>;

	// setup factory
	top_factory F1;
	F1.setup();
	byte_1024 F2;
	byte_1024.setup(&F1);
	sv_512 F3;
	F3.setup(&F1);
