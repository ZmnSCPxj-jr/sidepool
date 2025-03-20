#pragma once
#ifndef SIDEPOOL_CRYPTO_CSPRNG_HPP_
#define SIDEPOOL_CRYPTO_CSPRNG_HPP_

#include<cstdint>
#include<memory>

/* Forward-declare.  */
namespace Sidepool { template<typename a> class Io; }
namespace Sidepool { class Randomizer; }

namespace Sidepool::Crypto {

/** class Sidepool::Crypto::CSPRNG
 *
 * @desc A CSPRNG implementation.
 *
 * This uses `Sidepool::Io` actions; periodically
 * it re-seeds from the given `Sidepool::Randomizer`,
 * which can block, and this implementation will
 * serialize the seeding across multiple greenthreads.
 */
class CSPRNG {
private:
	class Impl;
	std::unique_ptr<Impl> pimpl;

public:
	/* Not default-constructible or copyable.  */
	CSPRNG() =delete;
	CSPRNG(CSPRNG const&) =delete;
	/* Movable tho.  */
	CSPRNG(CSPRNG&&);

	~CSPRNG();

	/* Normal construction.  */
	explicit
	CSPRNG(std::unique_ptr<Sidepool::Randomizer>);

	/* Type for returned blocks of randomness.  */
	struct Block {
		std::uint8_t data[32];
	};

	Sidepool::Io<std::unique_ptr<Block>>
	get_random();

	/** force_deterministic
	 *
	 * @desc For testing.
	 *
	 * In production, whenever we reseed,
	 * we also combine some "hail mary"
	 * not-very-random data, such as
	 * current time, in the hope that it
	 * will reduce issues in e.g.
	 * containerized environments where
	 * the OS random source is initially
	 * constant across all bootups of a
	 * specific container.
	 *
	 * Calling this function will disable
	 * the above "hail mary".
	 * Obviously you should do that right
	 * after constructing this.
	 *
	 * Note that this will still call
	 * the `Randomizer` so in the testing
	 * environment you still need a
	 * deterministic randomizer to seed.
	 */
	void force_deterministic();
};

}

#endif /* !defined(SIDEPOOL_CRYPTO_CSPRNG_HPP_) */
