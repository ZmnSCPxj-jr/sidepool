#pragma once
#ifndef SIDEPOOL_IO_HPP_
#define SIDEPOOL_IO_HPP_

#include<sstream>
#include<string>

namespace Sidepool {

template<typename a>
std::string stringify(a const& val) {
	auto ss = std::ostringstream();
	ss << val;
	return ss.str();
}

}

#endif /* !defined(SIDEPOOL_IO_HPP_) */
