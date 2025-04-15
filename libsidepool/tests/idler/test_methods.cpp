#ifdef HAVE_CONFIG_H
# include"config.h"
#endif
#undef NDEBUG
#include"Sidepool/Freed.hpp"
#include"Sidepool/Idler.hpp"
#include"Sidepool/Io.hpp"
#include<cassert>

int main() {
	auto cpp_idler =
		std::make_unique<Sidepool::Idler>()
	;

	auto flag = false;
	/* Test .start().  */
	{
		auto code = Sidepool::lift(
		).then([&]() {
			flag = true;
			return Sidepool::lift();
		});
		cpp_idler->start(
			std::move(code)
		);
	}
	/* flag should not be set yet, because
	 * the start function should wait for a
	 * idle period.
	 */
	assert(!flag);
	cpp_idler->now_idle();
	/* NOW flag should be set.  */
	assert(flag);

	flag = false;
	auto counter = 10;
	/* Test .yield() and Sidepool::Io tail-call
	 * optimization.
	 * The magic of JavaScript promises, and the
	 * Sidepool::Io class, is that they are
	 * really continuation-passing style, which
	 * means we get tail-call optimization for
	 * free, even if the C++ compiler has no
	 * tail-call optimization.
	 */
	{
		auto rec = std::function<
			Sidepool::Io<void>()
		>();
		rec = [&]() {
			if (counter == 0) {
				flag = true;
				return Sidepool::lift();
			}
			return cpp_idler->
				yield().then([&]() {
				--counter;
				/* recursion! */
				return rec();
			});
		};
		cpp_idler->start(rec());

		/* Complete execution.  */
		cpp_idler->now_idle();
		assert(counter == 0 && flag);
	}

	return 0;
}
