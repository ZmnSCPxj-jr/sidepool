#ifndef SIDEPOOL_FREED_HPP_
#define SIDEPOOL_FREED_HPP_

namespace Sidepool {

/** struct Sidepool::Freed
 *
 * @desc An empty structure that is thrown
 * by some `Sidepool::Io`-returning
 * functions if the sidepool instance is
 * shut down.
 *
 * In general, you do not need to actually
 * pay attention to this ***IF*** you do
 * RAII properly.
 * It largely only exists to ensure that
 * objects that need freeing ARE freed and
 * to coordinate freeing with the idler
 * interface.
 */
struct Freed { };

}

#endif // SIDEPOOL_FREED_HPP_
