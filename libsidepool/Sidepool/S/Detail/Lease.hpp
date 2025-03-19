#pragma once
#ifndef SIDEPOOL_S_DETAIL_LEASE_HPP_
#define SIDEPOOL_S_DETAIL_LEASE_HPP_

#include<memory>

/* Forward declare.  */
namespace Sidepool::S { class Lessee; }
namespace Sidepool::S::Detail { class Registry; }

namespace Sidepool::S::Detail {

/** class Sidepool::S::Detail::Lease
 *
 * @desc represents a lease on a registration to
 * be notified when a message is broadcast on an
 * `Sidepool::S::Bus`.
 *
 * The lease system allows for objects to
 * register to be notified on the
 * `Sidepool::S::Bus`, even if the object lifetime
 * is very short.
 *
 * A `Lease` object is a member of two different
 * singly-linked embedded lists:
 *
 * - A list owned by a `Sidepool::S::Detail::Registry`
 * - A list owned by a `Sidepool::S::Lessee`
 */
class Lease {
private:
	friend class Sidepool::S::Detail::Registry;
	friend class Sidepool::S::Lessee;

	/** The `next` field for the registry list.  */
	std::shared_ptr<Lease> next_reg;
	/** The `next` field for the lessor list.  */
	std::shared_ptr<Lease> next_lessor;

protected:

	/** Called by the lessor when the
	 * lessor is destroyed.
	 *
	 * The lease itself is not destroyed
	 * when the lessor is destroyed,
	 * but instead the next time the
	 * registry iterates over its list
	 * of leases, and it sees the
	 * specific lease has been cleared.
	 */
	virtual void clear() =0;

public:
	Lease() =default;

	/* Not copyable or movable.  */
	Lease(Lease const&) =delete;
	Lease(Lease&&) =delete;

	virtual ~Lease() =default;
};

}

#endif /* !defined(SIDEPOOL_S_DETAIL_LEASE_HPP_) */
