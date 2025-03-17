#ifdef HAVE_CONFIG_H
# include"config.h"
#endif
#undef NDEBUG
#include"Sidepool/stringify.hpp"
#include<cassert>

int main() {
	using Sidepool::stringify;
	assert(stringify(42) == std::string("42"));
	assert(stringify('a') == std::string("a"));
	return 0;
}
