#ifndef SIDEPOOL_IDLER_HPP_
#define SIDEPOOL_IDLER_HPP_

#include<functional>
#include<memory>

namespace Sidepool { template<typename a> class Io; }

namespace Sidepool {

/** class Sidepool::Idler
 *
 * @desc Concrete class to register a function to be
 * executed later.
 */
class Idler {
private:
	class Impl;
	std::unique_ptr<Impl> pimpl;

public:
	Idler();
	Idler(Idler&&) =default;

	Idler(Idler const&) =delete;

	~Idler();

	/** Start the given action the next time
	 * the process is idle.
	 *
	 * If the given greenthread throws a
	 * Sidepool::Freed, this ignores the
	 * exception, but all other exceptions
	 * will be rethrown outside of Io.
	 */
	void start(Sidepool::Io<void>);
	/** Called by a greenthread if it wants to
	 * either reset its C stack, or it wants to
	 * let other greenthreads run.
	 */
	Sidepool::Io<void> yield();
	/** Called by a greenthread to start a new
	 * greenthread.
	 * This should also yield so that the given
	 * greenthread can start execution.
	 *
	 * As it yields, it may also throw a
	 * Sidepool::Freed exception.
	 *
	 * If the given greenthread itself throws a
	 * Sidepool::Freed, this should ignore the
	 * exception, but all other exceptions
	 * should be reported as an unusual error.
	 */
	Sidepool::Io<void> fork(
		Sidepool::Io<void>
	);

	/** Call just before returning to the caller
	 * so that we can run everything we need to
	 * run.
	 */
	void now_idle();
};

}

#endif // SIDEPOOL_IDLER_HPP_
