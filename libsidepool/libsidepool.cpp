#if HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/Idler.hpp"
#include"Sidepool/Logger.hpp"
#include"Sidepool/Main.hpp"
#include"Sidepool/Randomizer.hpp"
#include"Sidepool/Saver.hpp"
#include"libsidepool.h"
#include<cerrno>
#include<cstddef>
#include<cstdint>
#include<memory>
#include<stdexcept>
#include<utility>

namespace {

/* Smart pointer for structures with a `free`
function.
*/
template<typename a>
struct LibsidepoolPtr {
private:
	a* pointer;

public:
	LibsidepoolPtr() : pointer(nullptr) { }
	LibsidepoolPtr(LibsidepoolPtr const&) =delete;
	LibsidepoolPtr(LibsidepoolPtr&& o) {
		pointer = o.pointer;
		o.pointer = nullptr;
	}
	LibsidepoolPtr(a* pointer_) : pointer(pointer_) { }

	LibsidepoolPtr<a>&
	operator=(LibsidepoolPtr&& o) {
		auto tmp = std::move(*this);
		pointer = o.pointer;
		o.pointer = nullptr;
		return *this;
	}

	explicit
	operator bool() const {
		return pointer != nullptr;
	}
	bool operator!() const {
		return pointer == nullptr;
	}

	~LibsidepoolPtr() {
		if (pointer && pointer->free) {
			pointer->free(pointer);
		}
	}

	a*& access() { return pointer; }
};

}

struct libsidepool_init {
	LibsidepoolPtr<libsidepool_randomizer> rand;
	LibsidepoolPtr<libsidepool_logger> logger;
	LibsidepoolPtr<libsidepool_idler> idler;
	LibsidepoolPtr<libsidepool_saver> saver;
	LibsidepoolPtr<libsidepool_math> math;
	LibsidepoolPtr<libsidepool_keykeeper> keykeeper;

	/* TODO: other required bits.  */

	libsidepool_init() =default;
	~libsidepool_init() =default;

	libsidepool_init(libsidepool_init&&) =delete;
	libsidepool_init(libsidepool_init const&) =delete;
};
struct libsidepool : public Sidepool::Main {
	libsidepool( std::unique_ptr<Sidepool::Idler> idler
		   , std::unique_ptr<Sidepool::Logger> logger
		   , std::unique_ptr<Sidepool::Randomizer> rand
		   , std::unique_ptr<Sidepool::Saver> saver
		   ) : Sidepool::Main( std::move(idler)
				     , std::move(logger)
				     , std::move(rand)
				     , std::move(saver)
				     ) { }
};

libsidepool_init*
libsidepool_init_start() {
	auto self = std::unique_ptr<libsidepool_init>();
	errno = 0;
	try {
		self = std::make_unique<libsidepool_init>();
	} catch (std::bad_alloc const& _) {
		errno = ENOMEM;
		return nullptr;
	} catch (...) {
		errno = EINVAL;
		return nullptr;
	}
	return self.release();
}

void
libsidepool_init_cancel(libsidepool_init* self_) {
	(void) std::unique_ptr<libsidepool_init>(self_);
}

void
libsidepool_init_set_randomizer( libsidepool_init* init
			       , libsidepool_randomizer* rand
			       ) {
	init->rand = LibsidepoolPtr<libsidepool_randomizer>(rand);
}
void
libsidepool_init_set_logger( libsidepool_init* init
			   , libsidepool_logger* logger
			   ) {
	init->logger = LibsidepoolPtr<libsidepool_logger>(logger);
}
void
libsidepool_init_set_idler( libsidepool_init* init
			  , libsidepool_idler* idler
			  ) {
	init->idler = LibsidepoolPtr<libsidepool_idler>(idler);
}
void
libsidepool_init_set_saver( libsidepool_init* init
			  , libsidepool_saver* saver
			  ) {
	init->saver = LibsidepoolPtr<libsidepool_saver>(saver);
}
void
libsidepool_init_set_math( libsidepool_init* init
			 , libsidepool_math* math
			 ) {
	init->math = LibsidepoolPtr<libsidepool_math>(math);
}
void
libsidepool_init_set_keykeeper( libsidepool_init* init
			      , libsidepool_keykeeper* keykeeper
			      ) {
	init->keykeeper = LibsidepoolPtr<libsidepool_keykeeper>(keykeeper);
}

libsidepool*
libsidepool_init_finish(libsidepool_init* self_) {
	auto self = std::unique_ptr<libsidepool_init>(self_);

	auto logger = std::unique_ptr<Sidepool::Logger>();

	auto rv = std::unique_ptr<libsidepool>();

	errno = 0;

	try {
		if (!self->logger) {
			logger = Sidepool::Logger::create_default();
			logger->debug("Logging will write to stderr.");
		} else {
			logger = Sidepool::Logger::create(self->logger.access());
			logger->debug("Got logger.");
		}

		if (!self->idler) {
			logger->error("Idler required.");
			errno = EINVAL;
			return nullptr;
		}
		auto idler = Sidepool::Idler::create(self->idler.access());
		logger->debug("Got idler.");

		if (!self->rand) {
			logger->error("Randomizer required.");
			errno = EINVAL;
			return nullptr;
		}
		auto rand = Sidepool::Randomizer::create(self->rand.access());
		logger->debug("Got randomizer.");

		if (!self->saver) {
			logger->error("Saver required.");
			errno = EINVAL;
			return nullptr;
		}
		auto saver = Sidepool::Saver::create(self->saver.access());
		logger->debug("Got saver.");

		rv = std::make_unique<libsidepool>(
			std::move(idler),
			std::move(logger),
			std::move(rand),
			std::move(saver)
		);

	} catch (std::bad_alloc const& _) {
		if (logger) {
			logger->error("Out of memory.");
		}
		errno = ENOMEM;
		return nullptr;
	} catch (std::exception const& e) {
		if (logger) {
			logger->error("Uncaught exception: %s", e.what());
		}
		errno = EINVAL;
		return nullptr;
	} catch (...) {
		if (logger) {
			logger->error("Uncaught exception of unknown type.");
		}
		errno = EINVAL;
		return nullptr;
	}

	if (rv) {
		rv->start();
	}

	return rv.release();
}

void
libsidepool_receive_message( libsidepool* main
			   , std::uint8_t peer[33]
			   , std::size_t len
			   , std::uint8_t const* msg
			   ) {
	main->receive_message(peer, len, msg);
}

/** Testing.  */
void
libsidepool_csprng_force_determinstic(libsidepool* main) {
	main->csprng_force_deterministic();
}

void
libsidepool_free(libsidepool* self_) {
	(void) std::unique_ptr<libsidepool>(self_);
}
