#pragma once
#ifndef SIDEPOOL_LOGGER_HPP_
#define SIDEPOOL_LOGGER_HPP_
#if HAVE_CONFIG_H
# include"config.h"
#endif

#include<memory>
#include<string>

extern "C" {
	struct libsidepool_logger;
}

namespace Sidepool {

class Logger {
protected:
	/* Concrete implementations should
	override these.
	*/
	virtual void log_debug(std::string const&) =0;
	virtual void log_info(std::string const&) =0;
	virtual void log_warn(std::string const&) =0;
	virtual void log_error(std::string const&) =0;

public:
	virtual ~Logger() { }

	/* Public interface, defined in the cpp
	 * file.
	 */
	void debug(char const* tpl, ...)
#if HAVE_ATTRIBUTE_PRINTF
		__attribute__ ((format (printf, 1, 2)))
#endif
	;
	void info(char const* tpl, ...)
#if HAVE_ATTRIBUTE_PRINTF
		__attribute__ ((format (printf, 1, 2)))
#endif
	;
	void warn(char const* tpl, ...)
#if HAVE_ATTRIBUTE_PRINTF
		__attribute__ ((format (printf, 1, 2)))
#endif
	;
	void error(char const* tpl, ...)
#if HAVE_ATTRIBUTE_PRINTF
		__attribute__ ((format (printf, 1, 2)))
#endif
	;

	/** Create a managed instance of
	 * `Sidepool::Logger` from a
	 * `libsidepool_logger*`
	 * This takes responsibility for the
	 * logger and sets the passed-in
	 * pointer to `nullptr` if it is
	 * able to create the object.
	 */
	static
	std::unique_ptr<Logger>
	create(libsidepool_logger*& var);

	/** Create a `Sidepool::Logger` that
	 * prints direcrly to stderr.
	 */
	static
	std::unique_ptr<Logger>
	create_default();
};

}

#endif /* !defined(SIDEPOOL_LOGGER_HPP_) */
