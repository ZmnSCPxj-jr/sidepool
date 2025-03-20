#if HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/Idler.hpp"
#include"Sidepool/Logger.hpp"
#include"Sidepool/Main.hpp"
#include"Sidepool/Mod/all.hpp"
#include"Sidepool/S/Bus.hpp"
#include"Sidepool/Util.hpp"
#include<vector>

namespace Sidepool {

class Main::Impl {
private:
	std::unique_ptr<Util> util;

	std::shared_ptr<void> mods;

public:
	Impl() =delete;
	Impl(Impl&&) =delete;
	Impl(Impl const&) =delete;

	Impl( std::unique_ptr<Idler> idler_
	    , std::unique_ptr<Logger> logger_
	    , std::unique_ptr<Randomizer> rand_
	    ) {
		util = std::make_unique<Util>(
			std::move(idler_),
			std::move(logger_),
			std::move(rand_)
		);

		mods = Mod::all(*util);

		/* TODO: raise init message. */
	}

	~Impl() {
		/* TODO: figure out if we
		need anything here.
		*/
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
	  , std::unique_ptr<Randomizer> rand
	  ) {
	pimpl = std::make_unique<Impl>(
		std::move(idler),
		std::move(logger),
		std::move(rand)
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
