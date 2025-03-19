#pragma once
#ifndef SIDEPOOL_S_DETAIL_TYPEDREGISTRY_HPP_
#define SIDEPOOL_S_DETAIL_TYPEDREGISTRY_HPP_

#include"Sidepool/Idler.hpp"
#include"Sidepool/Io.hpp"
#include"Sidepool/S/Detail/Registry.hpp"
#include"Sidepool/S/Detail/TypedLease.hpp"
#include<cstddef>
#include<exception>
#include<memory>

namespace Sidepool::S::Detail {

template<typename a>
class TypedRegistry : public Registry {
private:
	class Waiter {
	private:
		std::size_t count;
		std::exception_ptr except;

		std::function<void()> pass;
		std::function<void(std::exception_ptr)> fail;

		void complete() {
			if (except) {
				fail(except);
			} else {
				pass();
			}
		}
	public:
		Waiter() {
			count = 0;
			except = nullptr;
			pass = nullptr;
			fail = nullptr;
		}
		void incr() { ++count; }
		void decr() {
			--count;
			if (count == 0 && pass) {
				complete();
			}
		}
		void decr_fail(std::exception_ptr except_) {
			--count;
			if (!except) {
				except = except_;
			}
			if (count == 0 && pass) {
				complete();
			}
		}
		void set_func( std::function<void()> pass_
			     , std::function<void(std::exception_ptr)> fail_
			     ) {
			pass = std::move(pass_);
			fail = std::move(fail_);
			if (count == 0) {
				complete();
			}
		}
	};
public:
	TypedRegistry(TypedRegistry<a> const&) =delete;
	TypedRegistry(TypedRegistry<a>&&) =delete;

	TypedRegistry() =default;

	Sidepool::Io<void>
	raise( Sidepool::Idler& idler
	     , a val
	     ) {
		auto pval = std::make_shared<a>(
			std::move(val)
		);
		auto pwaiter = std::make_shared<Waiter>();
		auto act = Sidepool::lift();
		/* Go over our leases and build
		their actions.
		*/
		iterate([ pval
			, pwaiter
			, &act
			, &idler
			](Lease& l) {
			auto& tl = static_cast<TypedLease<a>&>(l);
			/* If the lease is already
			cleared, delete it and do
			nothing.
			*/
			if (!tl.fun) {
				return false;
			}
			pwaiter->incr();
			/* Get our action.  */
			auto my_act = tl.fun(*pval);
			/* Append the triggers on
			the action copletion.
			*/
			my_act = std::move(my_act).then([pwaiter, pval]() {
				pwaiter->decr();
				return Sidepool::lift();
			}).catch_all([pwaiter, pval](std::exception_ptr e) {
				pwaiter->decr_fail(e);
				return Sidepool::lift();
			});
			/* On the main action, fork
			the action of this lease.
			*/
			act += idler.fork(std::move(my_act));
			return true;
		});
		/* Add our final action, which is
		to feed our continuation to the
		waiter.
		*/
		act += Sidepool::Io<void>([ pwaiter
					  ] ( std::function<void()> pass
					    , std::function<void(std::exception_ptr)> fail
					    ) {
			pwaiter->set_func(
				std::move(pass),
				std::move(fail)
			);
		});
		return act;
	}
};

}

#endif /* !defined(SIDEPOOL_S_DETAIL_TYPEDREGISTRY_HPP_) */
