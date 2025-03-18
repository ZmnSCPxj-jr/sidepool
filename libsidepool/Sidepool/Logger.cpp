#if HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/Logger.hpp"
#include"Sidepool/String.hpp"
#include"libsidepool.h"
#include<cstdarg>
#include<ctime>
#include<iostream>

namespace Sidepool {

void Logger::debug(char const* tpl, ...) {
	std::va_list ap;

	va_start(ap, tpl);
	auto rv = String::vfmt(tpl, ap);
	va_end(ap);

	log_debug(rv);
}
void Logger::info(char const* tpl, ...) {
	std::va_list ap;

	va_start(ap, tpl);
	auto rv = String::vfmt(tpl, ap);
	va_end(ap);

	log_info(rv);
}
void Logger::warn(char const* tpl, ...) {
	std::va_list ap;

	va_start(ap, tpl);
	auto rv = String::vfmt(tpl, ap);
	va_end(ap);

	log_warn(rv);
}
void Logger::error(char const* tpl, ...) {
	std::va_list ap;

	va_start(ap, tpl);
	auto rv = String::vfmt(tpl, ap);
	va_end(ap);

	log_error(rv);
}

namespace {

class DefaultLogger : public Logger {
private:
	void log( char const* level
		, std::string const& msg
		) {
		auto raw_time = std::time(nullptr);
		auto cal_time = *std::gmtime(&raw_time);
		auto s_time = String::strftime(
			"%Y-%m-%d %a %H:%M:%S UTC%z",
			cal_time
		);

		std::cerr << s_time << " " << level
			  << ": " << msg << std::endl
			  ;
		std::cerr.flush();
	}
protected:
	void log_debug( std::string const& msg
		      ) override {
		log("DBG", msg);
	}
	void log_info( std::string const& msg
		      ) override {
		log("INF", msg);
	}
	void log_warn( std::string const& msg
		      ) override {
		log("WRN", msg);
	}
	void log_error( std::string const& msg
		      ) override {
		log("ERR", msg);
	}
};

}

std::unique_ptr<Logger>
Logger::create_default() {
	return std::make_unique<DefaultLogger>();
}

namespace {

class LoggerWrapper : public Logger {
private:
	libsidepool_logger* c_log;

public:
	LoggerWrapper() =delete;
	LoggerWrapper(LoggerWrapper const&) =delete;
	LoggerWrapper(LoggerWrapper&&) =delete;

	explicit
	LoggerWrapper(libsidepool_logger*& var) {
		c_log = var;
		var = nullptr;
	}

	~LoggerWrapper() {
		if (c_log->free) {
			c_log->free(c_log);
		}
	}
protected:
	void log_debug(std::string const& m) {
		c_log->debug(c_log, m.c_str());
	}
	void log_info(std::string const& m) {
		c_log->info(c_log, m.c_str());
	}
	void log_warn(std::string const& m) {
		c_log->warning(c_log, m.c_str());
	}
	void log_error(std::string const& m) {
		c_log->error(c_log, m.c_str());
	}
};

}

std::unique_ptr<Logger>
Logger::create(libsidepool_logger*& var) {
	return std::make_unique<LoggerWrapper>(var);
}

}
