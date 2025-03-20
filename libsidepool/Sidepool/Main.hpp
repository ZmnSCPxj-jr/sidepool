#pragma once
#ifndef SIDEPOOL_MAIN_HPP_
#define SIDEPOOL_MAIN_HPP_

#include<cstdint>
#include<memory>

namespace Sidepool { class Idler; }
namespace Sidepool { class Logger; }
namespace Sidepool { class Randomizer; }

namespace Sidepool {

/** class Sidepool::Main
 *
 * @desc The main class for the `libsidepool`
 * library.
 * The top-level god object that contains
 * everything.
 */
class Main {
private:
	class Impl;
	std::unique_ptr<Impl> pimpl;

public:
	/* Not default-constructible, moveable,
	or copyable.
	*/
	Main() =delete;
	Main(Main&&) =delete;
	Main(Main const&) =delete;

	/* Destructor is in implementation file.  */
	~Main();

	/* TODO: fill in all the required objects
	we need.
	*/
	explicit
	Main( std::unique_ptr<Idler> idler
	    , std::unique_ptr<Logger> logger
	    , std::unique_ptr<Randomizer> rand
	    );

	/** Call exactly once, just before we
	 * return the newly-constructed `Main`
	 * to C code.
	 *
	 * This is separate so that we can
	 * diagnose memory alloc errors at
	 * initialization separately from
	 * starting the thing.
	 */
	void start();

	/* Implementation of
	libsidepool_receive_message.
	*/
	void receive_message( std::uint8_t peer[33]
			    , std::size_t message_length
			    , std::uint8_t const* message
			    );

	/* Testing.  */
	void csprng_force_deterministic();
};

}

#endif /* !defined(SIDEPOOL_MAIN_HPP_) */
