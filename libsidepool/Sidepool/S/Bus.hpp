#pragma once
#ifndef SIDEPOOL_S_BUS_HPP_
#define SIDEPOOL_S_BUS_HPP_

#include"Sidepool/S/Detail/TypedLease.hpp"
#include"Sidepool/S/Detail/TypedRegistry.hpp"
#include"Sidepool/S/Lessee.hpp"
#include<functional>
#include<memory>
#include<typeindex>
#include<typeinfo>

namespace Sidepool { class Idler; }

namespace Sidepool::S {

/** class Sidepool::S::Bus
 *
 * @desc A class where you can subscribe a function
 * that accepts in some type, then when somebody
 * calls `raise` on that type, your function will
 * get called.
 *
 * The function must be return an `Io<void>`
 * function.
 * If multiple functions are subscribed, they will
 * be launched in parallel greenthreads.
 * `raise` will block (it also returns an
 * `Io<void>`) until all the functions complete
 * their `Io<void>`, and if one function throws,
 * then `raise` will throw the first exception
 * thrown.
 *
 * This is basically a signal bus, and is used to
 * allow the Sidepool library to be composed of
 * multiple somewhat-independent modules.
 */
class Bus {
private:
	class Impl;
	std::unique_ptr<Impl> pimpl;

	std::shared_ptr<Detail::Registry>
	get_registry( std::type_index
		    , std::function<std::shared_ptr<Detail::Registry>()> make_if_absent
		    );
	Sidepool::Idler& get_idler();

public:
	Bus() =delete;
	Bus(Bus const&) =delete;
	Bus(Bus&&) =delete;

	explicit
	Bus(Sidepool::Idler& idler);
	~Bus();

	template<typename a>
	Sidepool::Io<void>
	raise(a val) {
		auto r = get_registry(
			std::type_index(typeid(a)),
			nullptr
		);
		if (r) {
			auto& tr = static_cast<Detail::TypedRegistry<a>&>(*r);
			return tr.raise( get_idler()
				       , std::move(val)
				       );
		} else {
			return Sidepool::lift();
		}
	}
	template<typename a>
	void
	subscribe( Lessee& lessee
		 , std::function<Sidepool::Io<void>(a const& val)> cb
		 ) {
		auto r = get_registry(
			std::type_index(typeid(a)),
			[]() {
			return std::make_shared<Detail::TypedRegistry<a>>();
		});
		auto nlease = std::make_shared<Detail::TypedLease<a>>(std::move(cb));
		r->add_lease(nlease);
		lessee.add_lease(std::move(nlease));
	}
};

}

#endif /* !defined(SIDEPOOL_S_BUS_HPP_) */
