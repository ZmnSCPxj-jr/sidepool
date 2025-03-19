#ifndef SIDEPOOL_IDLER_HPP_
#define SIDEPOOL_IDLER_HPP_

#include"Sidepool/Io.hpp"

#include<memory>

extern "C" {
	struct libsidepool_idler;
}

namespace Sidepool {

/** class Sidepool::Idler
 *
 * @desc Abstract class to provide start(), yield(),
 * and fork() greenthread-management functions.
 */
class Idler {
public:
	virtual ~Idler() { }
	/** Start the given action the next time
	 * the process is idle.
	 *
	 * If the given greenthread throws a
	 * Sidepool::Freed, this should ignore the
	 * exception, but all other exceptions
	 * should be reported as an unusual error.
	 */
	virtual void start(Sidepool::Io<void>) =0;
	/** Called by a greenthread if it wants to
	 * either reset its C stack, or it wants to
	 * let other greenthreads run.
	 *
	 * Throws a Sidepool::Freed as an exception
	 * if the idler is freed (which should only
	 * happen if the main sidepool library
	 * instance is freed).
	 */
	virtual Sidepool::Io<void> yield() =0;
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
	virtual Sidepool::Io<void> fork(
		Sidepool::Io<void>
	) =0;

	/** Create a managed instance of
	 * `Sidepool::idler` from a
	 * `libsidepool_idler*`.
	 * This takes responsibility for the idler
	 * and sets the passed-in pointer to
	 * `nullptr` if it is able to create the
	 * object.
	 * If it throws, the passed-in pointer is
	 * not cleared.
	 */
	static
	std::unique_ptr<Idler>
	create(libsidepool_idler*& var);
};

}

#endif // SIDEPOOL_IDLER_HPP_
