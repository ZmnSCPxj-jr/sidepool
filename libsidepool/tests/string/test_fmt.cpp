#if HAVE_CONFIG_H
# include"config.h"
#endif
#undef NDEBUG
#include"Sidepool/String.hpp"
#include<cassert>

int main() {
	using Sidepool::String::fmt;
	assert(fmt("%s", "hello") == std::string("hello"));
	assert(fmt("%10s", "hello") == std::string("     hello"));
	assert(fmt("%-10s", "hello") == std::string("hello     "));
	assert(fmt("%c", 65) == std::string("A"));
	assert(fmt("%d", 65) == std::string("65"));
	assert(fmt("Some: %s with %d", "cat", 55) == std::string("Some: cat with 55"));
	return 0;
}
