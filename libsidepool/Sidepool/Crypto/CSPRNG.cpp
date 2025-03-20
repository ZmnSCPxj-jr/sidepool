#if HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/Crypto/CSPRNG.hpp"
#include"Sidepool/Crypto/SHA256.hpp"
#include"Sidepool/Crypto/chacha20_ietf.hpp"
#include"Sidepool/Freed.hpp"
#include"Sidepool/Io.hpp"
#include"Sidepool/Randomizer.hpp"
#include<cassert>
#include<cstring>
#include<ctime>
#include<exception>
#include<functional>
#include<queue>

namespace Sidepool::Crypto {

/** The number of times we will generate
 * keys before we reseed.
 *
 * This is expected to be small.
 * Pay attention that its type is
 * std::uint8_t.
 */
auto constexpr NUM_KEYS_BEFORE_RESEED = std::uint8_t(16);

class CSPRNG::Impl {
private:
	std::uint8_t key[32];
	std::uint8_t nonce[12];

	enum StateType {
		Normal,
		Reseeding
	};
	StateType state;

	struct Waiter {
		std::function<void()> pass;
		std::function<void(std::exception_ptr)> fail;
	};
	std::queue<Waiter> waiters;

	std::unique_ptr<Sidepool::Randomizer> randomizer;

	bool deterministic;

	Sidepool::Io<void>
	add_waiter() {
		return Sidepool::Io<void>([ this
					  ]( std::function<void()> pass
					   , std::function<void(std::exception_ptr)> fail
					   ) {
			waiters.push({
				std::move(pass),
				std::move(fail)
			});
		}).then([this]() {
			/* Check again.

			We do this since it is
			possible for there to be
			so many waiters that we
			hit NUM_KEYS_BEFORE_RESEED
			again.
			*/
			return wait_for_reseed();
		});
	}
	Sidepool::Io<void>
	reseed() {
		return Sidepool::lift().then([this]() {
			state = Reseeding;
			return randomizer->get();
		}).then([this](std::unique_ptr<std::uint8_t[]> entropy) {
			state = Normal;

			if (deterministic) {
				std::memcpy(key, entropy.get(), sizeof(key));
			} else {
				auto hasher = SHA256();
				auto time = std::time(nullptr);
				auto clock = std::clock();
				hasher.feed(entropy.get(), 32);
				hasher.feed((std::uint8_t*) &time, sizeof(time));
				hasher.feed((std::uint8_t*) &clock, sizeof(clock));
				/* TODO: hostname.  */
				auto h = std::move(hasher).finalize();
				std::memcpy(key, h.h, 32);
			}
			/* Reset the nonce counter.  */
			nonce[0] = 0;

			return Sidepool::Io<void>([ this
						  ]( std::function<void()> my_pass
						   , std::function<void(std::exception_ptr)> my_fail
						   ) {
				/* Run our pass first so that
				we get first dibs at the
				freshly reseeded key.
				*/
				my_fail = nullptr;
				std::move(my_pass)();
				/* Now let the waiters
				through.
				*/
				while (!waiters.empty()) {
					auto& front = waiters.front();
					auto pass = std::move(front.pass);
					waiters.pop();
					pass();
				}
			});
		});
	}

	Sidepool::Io<void>
	wait_for_reseed() {
		return Sidepool::lift().then([this]() {
			if (state == Reseeding) {
				/* Wait for reseed to finish.
				*/
				return add_waiter();
			}
			if (nonce[0] >= NUM_KEYS_BEFORE_RESEED) {
				return reseed();
			}
			/* nonce still safu.  */
			return Sidepool::lift();
		});
	}

public:
	~Impl() {
		/* Clear out the waiters.  */
		if (!waiters.empty()) {
			try {
				throw Sidepool::Freed();
			} catch (...) {
				auto e = std::current_exception();
				while (!waiters.empty()) {
					auto& front = waiters.front();
					auto fail = std::move(front.fail);
					waiters.pop();
					fail(e);
				}
			}
		}
		/* Destroy the randomizer early.
		This should cause any greenthread
		blocked on the randomizer to
		see `Sidepool::Freed` thrown.
		*/
		randomizer = nullptr;
	}

	explicit
	Impl(std::unique_ptr<Sidepool::Randomizer> randomizer_) {
		randomizer = std::move(randomizer_);
		/* Default deterministic to false.  */
		deterministic = false;
		/* We use nonce[0] as the counter for
		number of keys we have generated
		with this key.
		Set it to NUM_KEYS_BEFORE_RESEED so
		that the first call to `get_random`
		causes us to reseed.
		*/
		nonce[0] = NUM_KEYS_BEFORE_RESEED;
		/* Hail mary.  */
		nonce[1] = 'S';
		nonce[2] = 'i';
		nonce[3] = 'd';
		nonce[4] = 'e';
		nonce[5] = 'p';
		nonce[6] = 'o';
		nonce[7] = 'o';
		nonce[8] = 'l';
		nonce[9] = 'R';
		nonce[10] = 'N';
		nonce[11] = 'G';
	}

	Sidepool::Io<std::unique_ptr<Block>>
	get_random() {
		return wait_for_reseed().then([this]() {
			auto rv = std::make_unique<Block>();
			assert(nonce[0] < NUM_KEYS_BEFORE_RESEED);
			Sidepool::Crypto::chacha20_ietf(
				rv->data,
				sizeof(rv->data),
				key,
				nonce
			);

			/* Increment the nonce. */
			++nonce[0];

			return Sidepool::lift(
				std::move(rv)
			);
		});
	}

	void force_deterministic() {
		deterministic = true;
	}
};

CSPRNG::~CSPRNG() =default;

CSPRNG::CSPRNG(CSPRNG&& o) {
	pimpl = std::move(o.pimpl);
}
CSPRNG::CSPRNG(std::unique_ptr<Sidepool::Randomizer> r) {
	pimpl = std::make_unique<Impl>(
		std::move(r)
	);
}

Sidepool::Io<std::unique_ptr<CSPRNG::Block>>
CSPRNG::get_random() {
	assert(pimpl);
	return pimpl->get_random();
}
void
CSPRNG::force_deterministic() {
	assert(pimpl);
	return pimpl->force_deterministic();
}

};
