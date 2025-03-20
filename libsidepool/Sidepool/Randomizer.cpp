#if HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/Freed.hpp"
#include"Sidepool/Io.hpp"
#include"Sidepool/Randomizer.hpp"
#include"libsidepool.h"
#include<cassert>
#include<cstring>
#include<exception>
#include<utility>

namespace {

class RandomizerWrapper : public Sidepool::Randomizer
			{
private:
	typedef Sidepool::Randomizer::Entropy
		Entropy;

	libsidepool_randomizer* core;

	typedef std::unique_ptr<Entropy> Ret;
	typedef Sidepool::Io<Ret> IoRet;

	class Callback {
	private:
		std::function<void(Ret)> f_pass;
		std::function<void(std::exception_ptr)> f_fail;

	public:
		Callback() =delete;
		Callback(Callback&&) =delete;
		Callback(Callback const&) =delete;

		Callback( std::function<void(Ret)> f_pass_
			, std::function<void(std::exception_ptr)> f_fail_
			) {
			f_pass = std::move(f_pass_);
			f_fail = std::move(f_fail_);
		}

		static
		void pass( void* context
			 , std::uint8_t random[32]
			 ) {
			auto self = std::unique_ptr<Callback>(static_cast<Callback*>(context));
			auto pass = std::move(self->f_pass);
			self = nullptr;

			auto dat = std::make_unique<Entropy>();
			std::memcpy( dat->bytes
				   , random
				   , 32
				   );
			pass(std::move(dat));
		}
		static
		void fail(void* context) {
			auto self = std::unique_ptr<Callback>(static_cast<Callback*>(context));
			auto fail = std::move(self->f_fail);
			self = nullptr;

			try {
				throw Sidepool::Freed();
			} catch (...) {
				fail(std::current_exception());
			}
		}
	};

public:
	~RandomizerWrapper() override {
		if (core->free) {
			core->free(core);
		}
	}

	IoRet
	get() override {
		return IoRet([this
			     ]( std::function<void(Ret)> pass
			      , std::function<void(std::exception_ptr)> fail
			      ) {
			auto cb = std::make_unique<Callback>(
				std::move(pass),
				std::move(fail)
			);
			core->rand( core
				  , &Callback::pass
				  , &Callback::fail
				  , cb.release()
				  );
		});
	}
};

}
