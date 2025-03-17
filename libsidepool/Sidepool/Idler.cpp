#ifdef HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/Freed.hpp"
#include"Sidepool/Idler.hpp"
#include"Sidepool/Io.hpp"
#include"libsidepool.h"
#include<cassert>
#include<exception>
#include<functional>
#include<memory>

namespace {

class IdlerAdaptor : public Sidepool::Idler {
private:
	libsidepool_idler* c_intf;

	IdlerAdaptor() =delete;
	IdlerAdaptor(IdlerAdaptor const&) =delete;
	IdlerAdaptor(IdlerAdaptor&&) =delete;

	class Callback
		: public libsidepool_idle_callback {
	private:
		static void
		f_free(libsidepool_idle_callback* self_) {
			auto self = std::unique_ptr<Callback>(static_cast<Callback*>(self_));
			assert(self);
			self = nullptr;
		}
		static void
		f_invoke(libsidepool_idle_callback* self_) {
			auto self = std::unique_ptr<Callback>(static_cast<Callback*>(self_));
			assert(self);
			assert(self->func);
			self->func();
			self = nullptr;
		}

		Callback() =delete;
		Callback(Callback const&) =delete;
		Callback(Callback&&) =delete;

		std::function<void(void)> func;
	public:
		explicit
		Callback(std::function<void(void)> func_) : func(std::move(func_)) {
			free = &f_free;
			invoke = &f_invoke;
		}
	};

public:
	explicit
	IdlerAdaptor(libsidepool_idler*& c_intf_) {
		/* Once we have entered here, the
		 * memory for the IdlerAdaptor has
		 * been allocated and we can now
		 * take responsiblity for the c_intf.
		 *
		 * Assignment of PODs cannot throw,
		 * so the below is atomic with
		 * regards to exception-safety.
		 */
		c_intf = c_intf_;
		c_intf_ = nullptr;
	}

	~IdlerAdaptor() {
		if (c_intf->free) {
			c_intf->free(c_intf);
		}
	}

	virtual
	void start(Sidepool::Io<void> va) override {
		auto iop = std::make_shared<Sidepool::Io<void>>(std::move(va));
		auto cb = std::make_unique<Callback>([iop]() {
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
		/* Pass it on.  */
		c_intf->on_idle(c_intf, cb.release());
	}
	virtual
	Sidepool::Io<void> yield() override {
		return Sidepool::Io<void>([this]( std::function<void(void)> pass
						, std::function<void(std::exception_ptr)> fail
						) {
			auto cb = std::make_unique<Callback>(pass);
			c_intf->on_idle(c_intf, cb.release());
		});
	}
	virtual
	Sidepool::Io<void>
	fork(Sidepool::Io<void> va) override {
		auto iop = std::make_shared<Sidepool::Io<void>>(std::move(va));
		return Sidepool::lift().then([iop, this]() {
			start(std::move(*iop));
			return yield();
		});
	}
};

}

namespace Sidepool {

std::unique_ptr<Idler>
Idler::create(libsidepool_idler*& var) {
	return std::make_unique<IdlerAdaptor>(var);
}

}
