#pragma once
#ifndef SIDEPOOL_UTIL_HPP_
#define SIDEPOOL_UTIL_HPP_

#include"Sidepool/Crypto/CSPRNG.hpp"
#include"Sidepool/Idler.hpp"
#include"Sidepool/Io.hpp"
#include"Sidepool/Logger.hpp"
#include"Sidepool/Randomizer.hpp"
#include"Sidepool/S/Bus.hpp"
#include"Sidepool/S/Lessee.hpp"
#include"Sidepool/String.hpp"
#include<cstdarg>
#include<memory>

namespace Sidepool {

/** class Sidepool::Util
 *
 * @desc A convenient utility that just
 * wraps a `Sidepool::S::Bus`, a
 * `Sidepool::Idler`, and a
 * `Sidepool::Crypto::CSPRNG`.
 */
class Util {
private:
	std::unique_ptr<Idler> idler;
	std::unique_ptr<Logger> logger;
	std::unique_ptr<S::Bus> bus;
	std::unique_ptr<Crypto::CSPRNG> rng;

public:
	/* No default construct, move, copy.  */
	Util() =delete;
	Util(Util&&) =delete;
	Util(Util const&) =delete;

	~Util() =default;

	explicit
	Util( std::unique_ptr<Idler> idler_
	    , std::unique_ptr<Logger> logger_
	    , std::unique_ptr<Randomizer> rand_
	    ) {
		idler = std::move(idler_);
		logger = std::move(logger_);
		bus = std::make_unique<S::Bus>(*idler);
		rng = std::make_unique<Crypto::CSPRNG>(
			std::move(rand_)
		);
	}

	/* Sidepool::S::Bus */
	template<typename a>
	Sidepool::Io<void>
	raise(a val) {
		return bus->raise<a>(std::move(val));
	}
	template<typename a>
	void
	subscribe( S::Lessee& lessee
		 , std::function<Sidepool::Io<void>(a const& val)> cb
		 ) {
		return bus->subscribe<a>(
			lessee,
			std::move(cb)
		);
	}

	/* Sidepool::Idler */
	void start(Sidepool::Io<void> io) {
		idler->start(std::move(io));
	}
	Sidepool::Io<void> yield() {
		return idler->yield();
	}
	Sidepool::Io<void> fork(Sidepool::Io<void> io) {
		return idler->fork(std::move(io));
	}

	/* Sidepool::Logger */
	void debug(char const* tpl, ...)
#if HAVE_ATTRIBUTE_PRINTF
		__attribute__ ((format (printf, 1, 2)))
#endif
	{
		std::va_list ap;
		va_start(ap, tpl);
		auto s = String::vfmt(tpl, ap);
		va_end(ap);
		logger->debug("%s", s.c_str());
	}
	void info(char const* tpl, ...)
#if HAVE_ATTRIBUTE_PRINTF
		__attribute__ ((format (printf, 1, 2)))
#endif
	{
		std::va_list ap;
		va_start(ap, tpl);
		auto s = String::vfmt(tpl, ap);
		va_end(ap);
		logger->info("%s", s.c_str());
	}
	void warn(char const* tpl, ...)
#if HAVE_ATTRIBUTE_PRINTF
		__attribute__ ((format (printf, 1, 2)))
#endif
	{
		std::va_list ap;
		va_start(ap, tpl);
		auto s = String::vfmt(tpl, ap);
		va_end(ap);
		logger->warn("%s", s.c_str());
	}
	void error(char const* tpl, ...)
#if HAVE_ATTRIBUTE_PRINTF
		__attribute__ ((format (printf, 1, 2)))
#endif
	{
		std::va_list ap;
		va_start(ap, tpl);
		auto s = String::vfmt(tpl, ap);
		va_end(ap);
		logger->error("%s", s.c_str());
	}

	/* Sidepool::Crypto::CSPRNG */
	Sidepool::Io<std::unique_ptr<Sidepool::Crypto::CSPRNG::Block>>
	get_random() {
		return rng->get_random();
	}
	void
	force_deterministic() {
		rng->force_deterministic();
	}
};

}

#endif /* !defined(SIDEPOOL_UTIL_HPP_) */
