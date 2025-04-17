#if HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/Idler.hpp"
#include"Sidepool/Logger.hpp"
#include"Sidepool/Main.hpp"
#include"Sidepool/Math.hpp"
#include"Sidepool/Mod/all.hpp"
#include"Sidepool/Msg/ProvideMath.hpp"
#include"Sidepool/Msg/ProvideSaver.hpp"
#include"Sidepool/Msg/Start.hpp"
#include"Sidepool/S/Bus.hpp"
#include"Sidepool/Saver.hpp"
#include"Sidepool/Util.hpp"
#include<vector>

namespace Sidepool {

class Main::Impl {
private:
	std::unique_ptr<Util> util;
	std::unique_ptr<Saver> saver;
	std::shared_ptr<Math> math;

	std::shared_ptr<void> mods;

public:
	Impl() =delete;
	Impl(Impl&&) =delete;
	Impl(Impl const&) =delete;

	Impl( std::unique_ptr<Idler> idler_
	    , std::unique_ptr<Logger> logger_
	    , std::unique_ptr<Randomizer> rand_
	    , std::unique_ptr<Saver> saver_
	    , std::unique_ptr<Math> math_
	    ) {
		util = std::make_unique<Util>(
			std::move(idler_),
			std::move(logger_),
			std::move(rand_)
		);
		saver = std::move(saver_);
		math = std::move(math_);

		mods = Mod::all(*util);
	}

	void start() {
		util->start(Sidepool::lift().then([this]() {
			return util->raise(Msg::ProvideSaver{*saver});
		}).then([this]() {
			return util->raise(Msg::ProvideMath{math});
		}).then([this]() {
			return util->raise(Msg::Start{});
		}).then([this]() {
			util->debug("libsidepool started.");
			return Sidepool::lift();
		}));
		util->now_idle();
	}

	~Impl() {
		/* TODO: figure out if we
		need anything here.
		*/
	}

	void receive_message_core( std::uint8_t const peer[33]
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

		util->now_idle();
	}
	void receive_message( std::uint8_t const peer[33]
			    , std::size_t message_length
			    , std::uint8_t const* message
			    ) {
		try {
			receive_message_core(peer, message_length, message);
		} catch (std::bad_alloc const& _) {
			util->error("libsidepool_receive_message: Out-of-memory!");
		} catch (std::exception const& e) {
			util->error("libsidepool_receive_message: Uncaught exception: %s", e.what());
		} catch (...) {
			util->error("libsidepool_receive_message: Uncaught exception of unknown type.");
		}
	}
};

Main::Main( std::unique_ptr<Idler> idler
	  , std::unique_ptr<Logger> logger
	  , std::unique_ptr<Randomizer> rand
	  , std::unique_ptr<Saver> saver
	  , std::unique_ptr<Math> math
	  ) {
	pimpl = std::make_unique<Impl>(
		std::move(idler),
		std::move(logger),
		std::move(rand),
		std::move(saver),
		std::move(math)
	);
}

Main::~Main() =default;

void
Main::start() {
	pimpl->start();
}

void
Main::receive_message( std::uint8_t const peer[33]
		     , std::size_t message_length
		     , std::uint8_t const* message
		     ) {
	pimpl->receive_message(
		peer, message_length, message
	);
}

}
