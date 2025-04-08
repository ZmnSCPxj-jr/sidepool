#pragma once
#ifndef SIDEPOOL_SERDE_EOF_HPP_
#define SIDEPOOL_SERDE_EOF_HPP_

#include<stdexcept>

namespace Sidepool::SerDe {

struct Eof : public std::runtime_error {
	Eof() : std::runtime_error("End-of-file while deserializing data.") { }
};

}

#endif /* !defined(SIDEPOOL_SERDE_EOF_HPP_) */
