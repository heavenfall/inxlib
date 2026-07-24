cmake_minimum_required(VERSION 3.13)

# find include/inxlib/ -type f | awk -F/ '{print NF"/"$0}' | sort -n -t/ | cut -d/ -f3-
set(INXLIB_HEADERS
inxlib/inx.hpp
inxlib/types.hpp
inxlib/data/binary_tree.hpp
inxlib/data/bit_table.hpp
inxlib/data/mary_tree.hpp
inxlib/data/redblack_tree.hpp
inxlib/io/null.hpp
inxlib/io/transformers.hpp
inxlib/memory/area_factory.hpp
inxlib/memory/block_array.hpp
inxlib/memory/block_factory.hpp
inxlib/memory/factory.hpp
inxlib/memory/factory_adaptor.hpp
inxlib/memory/factory_pointer.hpp
inxlib/memory/indexed_factory.hpp
inxlib/memory/object.hpp
inxlib/memory/single_factory.hpp
inxlib/memory/slice_array.hpp
inxlib/memory/slice_factory.hpp
inxlib/memory/source_factory.hpp
inxlib/numeric/bits.hpp
inxlib/numeric/fixed_point.hpp
inxlib/numeric/int128.hpp
inxlib/numeric/math.hpp
inxlib/numeric/numeric_types.hpp
inxlib/util/bits.hpp
inxlib/util/functions.hpp
inxlib/util/iterator.hpp
inxlib/util/math.hpp
inxlib/util/numeric_types.hpp
inxlib/util/virtual_pointer.hpp
inxlib/util/xoshiro256.hpp
)
