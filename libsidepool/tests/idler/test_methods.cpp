#ifdef HAVE_CONFIG_H
# include"config.h"
#endif
#undef NDEBUG
#include"Sidepool/Freed.hpp"
#include"Sidepool/Idler.hpp"
#include"Sidepool/Io.hpp"
#include"libsidepool.h"
#include<cassert>

namespace {

class IdlerModel {
private:
	libsidepool_idle_callback* cbs;

public:
	IdlerModel() : cbs(nullptr) { }
	IdlerModel(IdlerModel const&) =delete;
	IdlerModel(IdlerModel&&) =delete;
	~IdlerModel() {
		while (cbs) {
			auto tmp = cbs;
			cbs = cbs->next;
			tmp->free(tmp);
		}
	}
	void run_callbacks() {
		/* invoke may cause on_idle
		 * to be called again!
		 *
		 * We should let callbacks be
		 * in separate lists from what
		 * we are currently processing
		 * so that we have better
		 * control of the timing of
		 * yield() and fork()
		 */
		auto my_cbs = cbs;
		cbs = nullptr;
		while (my_cbs) {
			auto tmp = my_cbs;
			my_cbs = my_cbs->next;
			tmp->invoke(tmp);
		}
	}
	void on_idle(libsidepool_idle_callback* cb) {
		cb->next = cbs;
		cbs = cb;
	}
};

/* This is a separate object so that it can share
the model with the test harness, so that the test
harness can invoke `run_callbacks`.
*/
class C_idler : public libsidepool_idler {
private:
	std::shared_ptr<IdlerModel> model;

	static
	void my_free(libsidepool_idler* self) {
		std::unique_ptr<C_idler>(
			static_cast<C_idler*>(self)
		);
	}
	static
	void my_on_idle( libsidepool_idler* self_
		       , libsidepool_idle_callback* cb 
		       ) {
		auto& self = *static_cast<C_idler*>(self_);
		self.model->on_idle(cb);
	}

public:
	C_idler() =delete;
	C_idler(C_idler const&) =delete;
	C_idler(C_idler&&) =delete;

	explicit
	C_idler( std::shared_ptr<IdlerModel> model_
	       ) : model(std::move(model_)) {
		user = nullptr;
		free = &my_free;
		on_idle = &my_on_idle;
	}
};

}

int main() {
	auto idler_model =
		std::make_shared<IdlerModel>()
	;
	auto c_idler = std::make_unique<C_idler>(
		idler_model
	);

	/* The idler model should have a use-count of
	 * 2: one for idler_model itself, the other
	 * for the c_idler wrapper.
	 */
	assert(idler_model.use_count() == 2);

	auto cpp_idler = std::unique_ptr<
				Sidepool::Idler
			>(
		nullptr
	);
	/* Convert the c_idler to C pointer,
	 * then load the C++ idler.
	 */
	{
		auto really_c_idler = static_cast<libsidepool_idler*>(
			c_idler.release()
		);
		/* Should now be owned by
		 * really_c_idler
		 */
		assert(really_c_idler);
		assert(!c_idler);

		cpp_idler = Sidepool::Idler::create(
			really_c_idler
		);

		/* Should now be owned by cpp_idler.  */
		assert(cpp_idler);
		assert(!really_c_idler);
	}

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
	idler_model->run_callbacks();
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

		/* Start requires one
		 * run_callbacks(), so counter and
		 * flag should not have changed yet.
		 */
		idler_model->run_callbacks();
		assert(counter == 10 && !flag);

		/* This loop needs to be in scope
		 * since we need rec() to be in
		 * scope while we are operating
		 * the loop.
		 */
		while (!flag) {
			auto sampled_counter = counter;
			idler_model->run_callbacks();
			/* Each run of the callnacks
			 * should have decremented
			 * counter, unless we
			 * are already at zero.
			 */
			assert( (sampled_counter == 0)
			     || (sampled_counter == counter + 1)
			      );
		}
	}

	/* Testing of free.  */
	flag = false;
	{
		auto code = Sidepool::lift(
		).then([&]() {
			return cpp_idler->yield();
		}).catching<Sidepool::Freed>([&](Sidepool::Freed const& e) {
			flag = true;
			throw e;
			return Sidepool::lift();
		});
		cpp_idler->start(std::move(code));
		/* start requires one
		 * run_callbacks().
		 */
		idler_model->run_callbacks();

		/* Now delete the cpp_idler and
		 * idler_model, which should
		 * cause the pending yield to
		 * throw Sidepool::Freed.
		 */
		cpp_idler = nullptr;
		/* The number of references to the
		 * idler_model should drop to 1.
		 */
		assert(idler_model.use_count() == 1);
		/* Now destroy the idler_model.  */
		idler_model = nullptr;

		/* The flag should be set, as per
		 * the catching<> clause.  */
		assert(flag);
	}

	return 0;
}
