#pragma once
#ifndef SIDEPOOL_S_DETAIL_REGISTRY_HPP_
#define SIDEPOOL_S_DETAIL_REGISTRY_HPP_

#include<functional>
#include<memory>

namespace Sidepool::S::Detail { class Lease; }

namespace Sidepool::S::Detail {

/** class Sidepool::S::Detail::Registry
 *
 * @desc Holds a list of leases.
 *
 * A lease is a function that has been registered
 * on some `Sidepool::S::Bus` to be called when
 * some specific type is raised on the bus.
 *
 * The registry holds all leases for a specific
 * type being raised.
 * See `TypedRegistry` also.
 */
class Registry {
private:
	/* We append to the list of leases.  */
	std::shared_ptr<Lease> first;
	Lease* last;

protected:
	/** Used by the derived `TypedRegistry`
	 * to iterate over leases.
	 *
	 * The given function should return true
	 * if the lease should be retained, and
	 * false if the lease sohuld be removed.
	 */
	void iterate(std::function<bool(Lease&)>);

public:
	Registry() : first(nullptr), last(nullptr) { }
	~Registry();

	Registry(Registry const&) =delete;
	Registry(Registry&&) =delete;

	void add_lease(std::shared_ptr<Lease> nlease);
};

}

#endif /* !defined(SIDEPOOL_S_DETAIL_REGISTRY_HPP_) */
