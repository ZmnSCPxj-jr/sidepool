#if HAVE_CONFIG_H
# include"config.h"
#endif
#undef NDEBUG
#include"Sidepool/S/Bus.hpp"
#include"Sidepool/S/Lessee.hpp"
#include"Testing/Idler.hpp"
#include<cassert>
#include<cstddef>

namespace {

struct Example { };

}

int main() {
	auto idler = Testing::Idler::make();
	auto bus = Sidepool::S::Bus(*idler);
	auto lessee1 = std::make_unique<Sidepool::S::Lessee>();
	auto lessee2 = std::make_unique<Sidepool::S::Lessee>();

	auto count = std::size_t(0);
	auto count2 = std::size_t(0);

	bus.subscribe<Example>(*lessee1, [&](Example const& _) {
		++count;
		return Sidepool::lift();
	});
	bus.subscribe<Example>(*lessee2, [&](Example const& _) {
		++count;
		++count2;
		return Sidepool::lift();
	});

	auto code = Sidepool::lift().then([&]() {
		assert(count == 0);

		return bus.raise(Example{});
	}).then([&]() {
		/* Both lessees should have
		triggered.
		*/
		assert(count == 2);
		assert(count2 == 1);

		/* Kill lessee1.  */
		lessee1 = nullptr;

		return bus.raise(Example{});
	}).then([&]() {
		/* lessee2 should have triggered,
		count and count2 have incremented
		once each.
		*/
		assert(count == 3);
		assert(count2 == 2);

		/* Kill lessee2.  */
		lessee2 = nullptr;

		return bus.raise(Example{});
	}).then([&]() {
		/* Should not have changed.
		*/
		assert(count == 3);
		assert(count2 == 2);

		return Sidepool::lift();
	});

	idler->start(std::move(code));
	idler->run();
	assert(count == 3);
	assert(count2 == 2);

	return 0;
}
