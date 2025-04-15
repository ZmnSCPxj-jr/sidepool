#ifdef HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/Freed.hpp"
#include"Sidepool/Idler.hpp"
#include"Sidepool/Io.hpp"
#include<cassert>
#include<exception>
#include<queue>

namespace Sidepool {

class Idler::Impl {
private:
	std::queue<std::function<void()>> to_call;

public:
	Impl() =default;

	Impl(Impl&&) =delete;
	Impl(Impl const&) =delete;

	~Impl() {
		now_idle();
	}

	void now_idle() {
		while (!to_call.empty()) {
			/* Calling `f` may cause additional
			 * functions to be enqueued.
			 * So move it out of the queue first
			 * and mutate the queue before calling
			 * into `f`.
			 */
			auto f = std::move(to_call.front());
			to_call.pop();
			if (f) {
				f();
			}
		}
	}

	void start(Sidepool::Io<void> io) {
		auto iop = std::make_shared<Sidepool::Io<void>>(std::move(io));
		to_call.emplace([iop]() {
			auto pass = [](){};
			auto fail = [](std::exception_ptr e) {
				try {
					if (e) {
						std::rethrow_exception(e);
					}
				} catch (Sidepool::Freed const& f) {
					/* Ignore.  */
				}
			};
			iop->run(std::move(pass), std::move(fail));
		});
	}

	Sidepool::Io<void> yield() {
		return Sidepool::Io<void>([this
					  ]( std::function<void()> pass
					   , std::function<void(std::exception_ptr)> fail
					   ) {
			to_call.emplace(std::move(pass));
		});
	}

	Sidepool::Io<void> fork( Sidepool::Io<void> io
			       ) {
		auto iop = std::make_shared<Sidepool::Io<void>>(std::move(io));
		return Sidepool::lift().then([iop, this]() {
			start(std::move(*iop));
			return yield();
		});
	}
};

Idler::Idler() : pimpl(std::make_unique<Impl>()) { }
Idler::~Idler() =default;

void Idler::start(Sidepool::Io<void> io) {
	assert(pimpl);
	pimpl->start(std::move(io));
}
Sidepool::Io<void> Idler::yield() {
	assert(pimpl);
	return pimpl->yield();
}
Sidepool::Io<void> Idler::fork(Sidepool::Io<void>io) {
	assert(pimpl);
	return pimpl->fork(std::move(io));
}

void Idler::now_idle() {
	assert(pimpl);
	pimpl->now_idle();
}

}
