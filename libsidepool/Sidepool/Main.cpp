#if HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/Idler.hpp"
#include"Sidepool/Logger.hpp"
#include"Sidepool/Main.hpp"
#include"Sidepool/S/Bus.hpp"
#include<vector>

namespace Sidepool {

class Main::Impl {
private:
	S::Bus bus;

	std::unique_ptr<Idler> idler;
	std::unique_ptr<Logger> logger;

public:
	Impl() =delete;
	Impl(Impl&&) =delete;
	Impl(Impl const&) =delete;

	Impl( std::unique_ptr<Idler> idler_
	    , std::unique_ptr<Logger> logger_
	    ) : bus(*idler_) {
		idler = std::move(idler_);
		logger = std::move(logger_);

		/* TODO: construct all our
		submodules and raise our
		initialization message.
		*/
	}

	~Impl() {
		/* Kill most objects first,
		then kill the idler last.
		*/
		logger = nullptr;

		idler = nullptr;
	}

	void receive_message( std::uint8_t peer[33]
			    , std::size_t message_length
			    , std::uint8_t const* message
			    ) {
		/* Create a vector copy of the given
		message.
		*/
		auto msg_v = std::vector<std::uint8_t>(
			message,
			message + message_length
		);
		/* TODO.  */
	}
};

Main::Main( std::unique_ptr<Idler> idler
	  , std::unique_ptr<Logger> logger
	  ) {
	pimpl = std::make_unique<Impl>(
		std::move(idler),
		std::move(logger)
	);
}

Main::~Main() =default;

void
Main::receive_message( std::uint8_t peer[33]
		     , std::size_t message_length
		     , std::uint8_t const* message
		     ) {
	pimpl->receive_message(
		peer, message_length, message
	);
}

}
