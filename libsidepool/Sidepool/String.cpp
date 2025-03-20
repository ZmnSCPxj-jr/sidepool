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

namespace {

std::uint8_t hexdigit(char c) {
	if ( ('a' <= c && c <= 'f')
	  || ('A' <= c && c <= 'F')
	   ) {
		return (c & 0xF) + 9;
	} else if ('0' <= c && c<= '9') {
		return (c & 0xF);
	} else {
		throw HexParsingError();
	}
}

}

std::vector<std::uint8_t>
parsehex(std::string const& s) {
	auto rv = std::vector<std::uint8_t>();
	auto p = s.begin();
	while (p != s.end()) {
		auto p2 = p;
		++p2;
		if (p2 == s.end()) {
			throw HexParsingError();
		}
		auto hi = hexdigit(*p);
		auto lo = hexdigit(*p2);
		auto byte = (hi << 4) | (lo);
		rv.push_back(byte);

		p = p2;
		++p;
	}
	return rv;
}

}

