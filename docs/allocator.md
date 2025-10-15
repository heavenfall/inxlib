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

Concept `ElemFreeFactory` is an `ArrayFactory` that supports `deallocate` function calls.

Concept `ReleaseFactory` is a `Factory` that supports `release()` and `release(bool)` function,
with `release(bool)` will releases all allocations to upstream if true, otherwise would logically release memory allocations.
A logical release is useful if an upstream resouce has already released its memory, thus calling to upstream
or using the current allocators may be ill-formed, the `release(false)` would simply forget any memory it has or
had issued.

Concept `ReclaimFactory` is a `ReleaseFactory` that supports `reclaim` function calls,
which will logically release all allocations given by this factory, but instead of returning resouces to
upstream like `release(true)` or resetting memory state like `release(false)`, `reclaim()` would instead
try to reuse its memory resources as much as possible.
This does not mean that no memory would be released, only that it would reuse what it can.

Function `upstream(Factory&)` return the `upstream` of provided, and `upstream<Factory>(Factory&)` returns the template class upstream of provided upstream, i.e. chains until reaching template class.

## Setup and Use

Factories are designed around chain-inheritance.
For example: `single_factory<malloc_factory, sizeof(int)>` has the top-level factory `single_factory`
inherit the `malloc_factory` as its upstream, resulting in `single_factory` serving int sized memory
blocks using the malloc upstream blocks.

As factories can be given run-time parameters, this is initalized with the `setup()` function call.
The end factory will require the user to call `setup(args...)`, with args being a set of arguments
used at runtime.
The methodolgy is that each factory will use 1 or 0 arguments (taken from the front), and pass the rest
to its upstream setup function.
It will return true if factory initalization is correct, in which case the user can then use the allocator.
If any of the factories return false, then the allocator is not in a valid state for use.

## Core factories

The primary method of allocating memory is through the `malloc_factory` in header `inxlib/memory/malloc_factory.hpp`.
This factory is a `BytesFactory`, and is non-owning, thus `deallocate` must be called for every
`allocate` else a memory leak will occur.

The `single_factory` in header `inxlib/memory/single_factory.hpp` provides a basic way to allocate a single memory
of set size from an `ArrayFactory`.
This primarily is intended for use with a `ByteFactory` type.

## Factory Pointer

The header `inxlib/memory/factory_chain.hpp` gives support methods for how factories chain together, primarily
a pointer wrapper to a factory.
This wrapper is important as without it, there would be no method to split factory connections.
For example:
given some `ByteFactory Src`, for example a `bump_factory` that gives memory allocations out from larger block,
the user would normally have to allocated `Src` sepretatly between two seperate `single_factory`, but by using
`factory_pointer<Src>`, a user can use the following code:

	Src src;
	src.setup();
	single_factory<factory_pointer<src>, sizeof(int)> fact_int;
	single_factory<factory_pointer<src>, sizeof(double)> fact_double;
	fact_int.setup(&src);
	fact_double.setup(&src);

This setup will have `fact_int` and `fact_double` both share `src` as their upstream.

## Factory Adaptors

The header `inxlib/memory/factory_adaptor.hpp` provides special adaptors.
While these adaptors behave similiarly to factories, they differ in what they provide the user.
A normal factory focuses on how to shape memory allocations or provide resource, while an adaptor is
designed to add functionality to an existing factory without changing the resouces.

The two provided adaptors for end users are the `reuse_adaptor` and `reclaim_adaptor`.
The `reuse_adaptor` requires a `SingleFactory` upstream of element size at least big enough to store a pointer (8 bytes normally).
This adaptor instead of passing `destory` calls to upsteam will store the address and re-issue it on a `create` call.

The `reclaim_adaptor` takes a `ByteFactory` upstream and adds support for memory reclaim, which adds
a `release` and `reclaim` function call.
It adds book keeping to all memory allocations that allows a `ByteFactory` to reclaim resources given out
from `allocate`.

The `overflow_pattern` is not quite an adaptor, but a pattern.
Patterns are designed here as an adaptor of sorts for use interally by factories.
The `overflow_pattern` takes both an `Upstream` and `Overflow`, where `Overflow` is a seperate `ByteFactory`
for extra allocations that do not fit the default 
