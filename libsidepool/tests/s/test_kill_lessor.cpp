#if HAVE_CONFIG_H
# include"config.h"
#endif
#undef NDEBUG
#include"Sidepool/S/Bus.hpp"
#include"Sidepool/S/Lessor.hpp"
#include"Testing/Idler.hpp"
#include<cassert>
#include<cstddef>

namespace {

struct Example { };

}

int main() {
	auto idler = Testing::Idler::make();
	auto bus = Sidepool::S::Bus(*idler);
	auto lessor1 = std::make_unique<Sidepool::S::Lessor>();
	auto lessor2 = std::make_unique<Sidepool::S::Lessor>();

	auto count = std::size_t(0);
	auto count2 = std::size_t(0);

	bus.subscribe<Example>(*lessor1, [&](Example const& _) {
		++count;
		return Sidepool::lift();
	});
	bus.subscribe<Example>(*lessor2, [&](Example const& _) {
		++count;
		++count2;
		return Sidepool::lift();
	});

	auto code = Sidepool::lift().then([&]() {
		assert(count == 0);

		return bus.raise(Example{});
	}).then([&]() {
		/* Both lessors should have
		triggered.
		*/
		assert(count == 2);
		assert(count2 == 1);

		/* Kill lessor1.  */
		lessor1 = nullptr;

		return bus.raise(Example{});
	}).then([&]() {
		/* lessor2 should have triggered,
		count and count2 have incremented
		once each.
		*/
		assert(count == 3);
		assert(count2 == 2);

		/* Kill lessor2.  */
		lessor2 = nullptr;

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
