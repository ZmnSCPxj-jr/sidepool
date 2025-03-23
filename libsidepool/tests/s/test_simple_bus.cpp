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

struct Example {
};
struct Example2 {
};

}

int main() {
	auto idler = Testing::Idler::make();
	auto lessee = Sidepool::S::Lessee();
	auto bus = Sidepool::S::Bus();

	auto count = std::size_t(0);

	bus.subscribe<Example>(lessee, [&](Example const&) {
		++count;
		return Sidepool::lift();
	});

	auto code = Sidepool::lift().then([&]() {
		assert(count == 0);

		return bus.raise(Example{});
	}).then([&]() {
		assert(count == 1);

		return bus.raise(Example{});
	}).then([&]() {
		assert(count == 2);

		/* Example2 is not same type as
		Example1.
		*/
		return bus.raise(Example2{});
	}).then([&]() {
		assert(count == 2);

		return Sidepool::lift();
	});

	idler->start(std::move(code));
	idler->run();

	assert(count == 2);

	return 0;
}
