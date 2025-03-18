#ifdef HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/String.hpp"
#include<cstddef>
#include<cstdio>

namespace Sidepool::String {

std::string fmt(char const* tpl, ...) {
	std::va_list ap;

	auto rv = std::string();

	va_start(ap, tpl);
	rv = vfmt(tpl, ap);
	va_end(ap);

	return rv;
}

std::string vfmt( char const* tpl
		, std::va_list ap_orig
		) {
	std::va_list ap;

	auto written = std::size_t(0);
	auto size = std::size_t(64);
	auto buf = std::unique_ptr<char[]>();

	do {
		if (size <= written) {
			size = written + 1;
		}
		buf = std::make_unique<char[]>(size);
		va_copy(ap, ap_orig);
		written = std::size_t(
			vsnprintf(buf.get(), size, tpl, ap)
		);
		va_end(ap);
	} while (size <= written);

	return std::string(buf.get());
}

std::string strftime(char const* tpl, std::tm const& tm) {
	auto written = std::size_t(0);
	auto size = std::size_t(64);
	auto buf = std::unique_ptr<char[]>();

	do {
		if (size <= written) {
			size = written + 1;
		}
		buf = std::make_unique<char[]>(size);
		written = std::strftime(
			buf.get(),
			size,
			tpl,
			&tm
		);
	} while (size <= written);

	return std::string(buf.get());
}

}

