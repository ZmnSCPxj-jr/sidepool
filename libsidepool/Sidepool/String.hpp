#pragma once
#ifndef SIDEPOOL_STRING_HPP_
#define SIDEPOOL_STRING_HPP_
#ifdef HAVE_CONFIG_H
# include"config.h"
#endif
#include<cstdarg>
#include<string>

namespace Sidepool::String {

std::string fmt(char const* tpl, ...)
#if HAVE_ATTRIBUTE_FORMAT
	__attribute__ ((format (printf, 1, 2)))
#endif
;
std::string vfmt(char const* tpl, std::va_list ap);

}

#endif /* !defined(SIDEPOOL_STRING_HPP_) */
