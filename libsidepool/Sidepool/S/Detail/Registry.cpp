#if HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/S/Detail/Lease.hpp"
#include"Sidepool/S/Detail/Registry.hpp"
#include<cassert>
#include<utility>

namespace Sidepool::S::Detail {

void
Registry::iterate(std::function<bool(Lease&)> f) {
	auto ptr = first;
	auto prev_ptr = std::shared_ptr<Lease>(nullptr);
	while (ptr) {
		auto ok = f(*ptr);
		if (ok) {
			/* Retain lease.  */
			prev_ptr = std::move(ptr);
			ptr = prev_ptr->next_reg;
			continue;
		}
		/* Delete the lease pointed by
		ptr.
		*/
		/* Handle the list-pointer for
		this lease.
		*/
		if (!prev_ptr) {
			first = ptr->next_reg;
		} else {
			prev_ptr->next_reg = ptr->next_reg;
		}
		/* Handle the case where this is
		the last lease.
		*/
		if (last == ptr.get()) {
			last = prev_ptr.get();
		}
		/* Now update ptr (prev_ptr does
		not change).
		*/
		ptr = std::move(ptr->next_reg);
	}
}

void
Registry::add_lease(std::shared_ptr<Lease> nlease) {
	assert(nlease);
	if (!first) {
		first = std::move(nlease);
		last = first.get();
	} else {
		last->next_reg = std::move(nlease);
		last = last->next_reg.get();
	}
}

Registry::~Registry() {
	last = nullptr;
	while (first) {
		first->clear();
		first = std::move(first->next_reg);
	}
}

}
