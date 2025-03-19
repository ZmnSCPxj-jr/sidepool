#if HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/S/Detail/Lease.hpp"
#include"Sidepool/S/Lessor.hpp"
#include<cassert>
#include<utility>

namespace Sidepool::S {

void
Lessor::add_lease(
		std::shared_ptr<Detail::Lease> nlease
) {
	assert(nlease);
	nlease->next_lessor = std::move(top);
	top = std::move(nlease);
}

Lessor::~Lessor() {
	while (top) {
		top->clear();
		top = std::move(top->next_lessor);
	}
}

}
