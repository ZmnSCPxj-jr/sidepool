#pragma once
#ifndef SIDEPOOL_S_DETAIL_TYPEDLEASE_HPP_
#define SIDEPOOL_S_DETAIL_TYPEDLEASE_HPP_

#include"Sidepool/S/Detail/Lease.hpp"
#include<functional>

namespace Sidepool { template<typename a> class Io; }
namespace Sidepool::S::Detail { template<typename a> class TypedRegistry; }

namespace Sidepool::S::Detail {

/** class Sidepool::S::Detail::TypedLease<a>
 *
 * @desc A concrete lease type that holds a
 * registered function that should be called
 * when the corresponding type is raised on
 * the `Sidepool::S::Bus`.
 */
template<typename a>
class TypedLease : public Lease {
private:
	friend class TypedRegistry<a>;
	std::function<Sidepool::Io<void>(a const&)> fun;
protected:
	void clear() override {
		fun = nullptr;
	}
public:
	TypedLease() =delete;
	TypedLease(TypedLease const&) =delete;
	TypedLease(TypedLease&&) =delete;

	explicit
	TypedLease( std::function<Sidepool::Io<void>(a const&)> fun_
		  ) : fun(std::move(fun_)) { }
};

}

#endif /* !defined(SIDEPOOL_S_DETAIL_TYPEDLEASE_HPP_) */
