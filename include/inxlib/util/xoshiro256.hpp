/*  Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>.*/

#ifndef INXLIB_UTIL_XOSHIRO256_HPP
#define INXLIB_UTIL_XOSHIRO256_HPP

#include <bit>
#include <inxlib/inx.hpp>
#include <limits>
#include <random>

/* This is xoshiro256** 1.0, one of our all-purpose, rock-solid
   generators. It has excellent (sub-ns) speed, a state (256 bits) that is
   large enough for any parallel application, and it passes all tests we
   are aware of.

   For generating just floating-point numbers, xoshiro256+ is even faster.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */

namespace inx::util {

class xoshiro256
{
public:
	using result_type = uint64_t;
	xoshiro256()
	  : s{}
	{
	}
	xoshiro256(std::uniform_random_bit_generator auto& seeder) { seed(seeder); }

	void seed(std::uniform_random_bit_generator auto& seeder)
	{
		using srt = std::remove_cvref_t<decltype(seeder)>::result_type;
		for (int i = 0; i < 4; ++i) {
			if constexpr (sizeof(result_type) <= sizeof(srt)) {
				s[i] = seeder();
			} else {
				result_type x{};
				for (size_t j = 0; j < sizeof(result_type) / sizeof(srt); ++j) {
					x <<= j * (8 * sizeof(srt));
					x |= seeder();
				}
				s[i] = x;
			}
		}
	}

	consteval static result_type min() noexcept { return std::numeric_limits<result_type>::min(); }
	consteval static result_type max() noexcept { return std::numeric_limits<result_type>::max(); }

	result_type operator()() noexcept
	{
		const result_type result = std::rotl(s[1] * 5, 7) * 9;

		const result_type t = s[1] << 17;

		s[2] ^= s[0];
		s[3] ^= s[1];
		s[1] ^= s[2];
		s[0] ^= s[3];

		s[2] ^= t;

		s[3] = std::rotl(s[3], 45);

		return result;
	}

	/**
	 * Progress 2^128 rng states.
	 */
	xoshiro256& jump()
	{
		constexpr std::array<uint64_t, 4> JUMP{
		  0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c};

		uint64_t s0 = 0;
		uint64_t s1 = 0;
		uint64_t s2 = 0;
		uint64_t s3 = 0;
		for (int i = 0; i < JUMP.size(); i++) {
			for (int b = 0; b < 64; b++) {
				if (JUMP[i] & (static_cast<uint64_t>(1) << b)) {
					s0 ^= s[0];
					s1 ^= s[1];
					s2 ^= s[2];
					s3 ^= s[3];
				}
				(*this)();
			}
		}
		s[0] = s0;
		s[1] = s1;
		s[2] = s2;
		s[3] = s3;
		return *this;
	}

	/* This is the long-jump function for the generator. It is equivalent to
	2^192 calls to next(); it can be used to generate 2^64 starting points,
	from each of which jump() will generate 2^64 non-overlapping
	subsequences for parallel distributed computations. */
	/**
	 * Progress 2^192 rng states.
	 */
	xoshiro256& long_jump()
	{
		constexpr std::array<uint64_t, 4> JUMP{
		  0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241, 0x39109bb02acbe635};

		uint64_t s0 = 0;
		uint64_t s1 = 0;
		uint64_t s2 = 0;
		uint64_t s3 = 0;
		for (int i = 0; i < JUMP.size(); i++) {
			for (int b = 0; b < 64; b++) {
				if (JUMP[i] & (static_cast<uint64_t>(1) << b)) {
					s0 ^= s[0];
					s1 ^= s[1];
					s2 ^= s[2];
					s3 ^= s[3];
				}
				(*this)();
			}
		}

		s[0] = s0;
		s[1] = s1;
		s[2] = s2;
		s[3] = s3;
		return *this;
	}

private:
	result_type s[4];
};

} // namespace inx::util

#endif // INXLIB_UTIL_XOSHIRO256_HPP
