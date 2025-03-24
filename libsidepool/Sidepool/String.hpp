#pragma once
#ifndef SIDEPOOL_STRING_HPP_
#define SIDEPOOL_STRING_HPP_
#ifdef HAVE_CONFIG_H
# include"config.h"
#endif
#include<cstdarg>
#include<cstdint>
#include<ctime>
#include<ostream>
#include<stdexcept>
#include<string>
#include<vector>

namespace Sidepool::String {

std::string fmt(char const* tpl, ...)
#if HAVE_ATTRIBUTE_FORMAT
	__attribute__ ((format (printf, 1, 2)))
#endif
;
std::string vfmt(char const* tpl, std::va_list ap);

std::string strftime(char const* tpl, std::tm const& tm);

struct HexParsingError : public std::runtime_error {
	HexParsingError() : std::runtime_error("Invalid hex string") { }
};

std::vector<std::uint8_t> parsehex(std::string const&);

void printhex( std::ostream&
	     , std::uint8_t const* b
	     , std::uint8_t const* e
	     );
inline
void printhex( std::ostream& os
	     , std::vector<std::uint8_t> const& v
	     ) {
	printhex(os, &*v.begin(), &*v.end());
}


}

#endif /* !defined(SIDEPOOL_STRING_HPP_) */
