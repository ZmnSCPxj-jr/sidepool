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

		std::shared_ptr<a> pval;

		std::function<void()> pass;
		std::function<void(std::exception_ptr)> fail;

		void complete() {
			pval = nullptr;
			if (except) {
				fail(except);
			} else {
				pass();
			}
		}
	public:
		Waiter() =delete;
		Waiter(Waiter&&) =delete;
		Waiter(Waiter const&) =delete;
		explicit
		Waiter(std::shared_ptr<a> pval_) {
			count = 0;
			except = nullptr;
			pval = std::move(pval_);
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
	raise( a val
	     ) {
		auto pval = std::make_shared<a>(
			std::move(val)
		);
		/* Construct the waiter and give it a copy
		of the shared pointer to value.
		This keeps the value passed to the callback
		alive until the callbacks have completed.
		*/
		auto pwaiter = std::make_shared<Waiter>(
			pval
		);
		auto act = Sidepool::lift();
		/* Go over our leases and build
		their actions.
		*/
		iterate([ pval
			, pwaiter
			, &act
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
			auto pmy_act = std::make_shared<Sidepool::Io<void>>(std::move(my_act));
			/* On the main action, append a
			new action to call into our
			action.
			*/
			act = std::move(act).then([ pwaiter
						  , pmy_act
						  ]() {
				pmy_act->run( [pwaiter]() {
					pwaiter->decr();
				}, [pwaiter](std::exception_ptr e) {
					pwaiter->decr_fail(e);
				});
				return Sidepool::lift();
			});
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
