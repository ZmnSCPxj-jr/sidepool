#pragma once
#ifndef SIDEPOOL_S_LESSOR_HPP_
#define SIDEPOOL_S_LESSOR_HPP_

#include<memory>

/* Forward-declare.  */
namespace Sidepool::S { class Bus; }
namespace Sidepool::S::Detail { class Lease; }

namespace Sidepool::S {

/** class Sidepool::S::Lessor
 *
 * @desc An object that, if it is destroyed,
 * automatically deregisters all registered
 * functions on a specific `Sidepool::S::Bus`.
 *
 * It is safe for the lessor to outlive the
 * bus(es) it is used with; memory will be
 * retained uselessly until the lessor is
 * also destroyed, but there would be
 * no memory leak.
 */
class Lessor {
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
	Lessor() : top(nullptr) { }
	/* Disallow copy and move.  */
	Lessor(Lessor const&) =delete;
	Lessor(Lessor&&) =delete;

	~Lessor();
};

}

#endif /* !defined(SIDEPOOL_S_LESSOR_HPP_) */
