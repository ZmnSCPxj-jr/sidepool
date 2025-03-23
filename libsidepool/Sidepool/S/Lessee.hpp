#pragma once
#ifndef SIDEPOOL_S_LESSEE_HPP_
#define SIDEPOOL_S_LESSEE_HPP_

#include<memory>

/* Forward-declare.  */
namespace Sidepool::S { class Bus; }
namespace Sidepool::S::Detail { class Lease; }

namespace Sidepool::S {

/** class Sidepool::S::Lessee
 *
 * @desc An object that, if it is destroyed,
 * automatically deregisters all registered
 * functions on a specific `Sidepool::S::Bus`.
 *
 * It is safe for the lessee to outlive the
 * bus(es) it is used with; memory will be
 * retained uselessly until the lessee is
 * also destroyed, but there would be
 * no memory leak.
 */
class Lessee {
private:
	std::shared_ptr<Detail::Lease> top;

	friend class Sidepool::S::Bus;
	/* Called by the bus when a new
	 * message-triggered function is
	 * registered.
	 */
	void add_lease(std::shared_ptr<Detail::Lease>);

public:
	explicit
	Lessee() : top(nullptr) { }
	/* Disallow copy and move.  */
	Lessee(Lessee const&) =delete;
	Lessee(Lessee&&) =delete;

	~Lessee();
};

}

#endif /* !defined(SIDEPOOL_S_LESSEE_HPP_) */
