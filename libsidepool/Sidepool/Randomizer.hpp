#pragma once
#ifndef SIDEPOOL_RANDOMIZER_HPP_
#define SIDEPOOL_RANDOMIZER_HPP_

#include<cstdint>
#include<memory>

extern "C" {
	struct libsidepool_randomizer;
}

namespace Sidepool { template<typename a> class Io; }

namespace Sidepool {

/** class Sidepool::Randomizer
 *
 * @desc A blocking source of high-quality
 * random numbers.
 *
 * Each call to the `get` function should
 * return an array of 32 random bytes.
 */
class Randomizer {
public:
	/** The type for a batch of 32
	 * random bytes.
	 */
	struct Entropy {
		std::uint8_t bytes[32];
	};

	virtual ~Randomizer() { }

	/** Get 32 random bytes (i.e. an
	 * Entropy).
	 *
	 * Must not return `nullptr`.
	 */
	virtual
	Sidepool::Io<std::unique_ptr<Entropy>>
	get() =0;

	/** Create a managed instance of
	 * `Sidepool::Randomizer` from a
	 * `libsidepool_randomizer*`
	 * This takes responsibilty for the
	 * randomizer and sets the passed-in pointer
	 * to `nullptr` if it is able to create the
	 * object.
	 */
	static
	std::unique_ptr<Randomizer>
	create(libsidepool_randomizer*&);
};

}

#endif /* !defined(SIDEPOOL_RANDOMIZER_HPP_) */
