#if HAVE_CONFIG_H
# include"config.h"
#endif
#undef NDEBUG
#include"Sidepool/String.hpp"
#include<cassert>

int main() {
	using Sidepool::String::parsehex;
	typedef Sidepool::String::HexParsingError
		HexParsingError;

	{
		auto dat = parsehex("");
		assert(dat.size() == 0);
	}
	{
		auto dat = parsehex("4C");
		assert(dat.size() == 1);
		assert(dat[0] == 0x4C);
	}
	{
		auto dat = parsehex("04f0");
		assert(dat.size() == 2);
		assert(dat[0] == 0x04);
		assert(dat[1] == 0xf0);
	}

	{
		auto flag = false;
		try {
			parsehex("1");
		} catch (HexParsingError const&) {
			flag = true;
		}
		assert(flag);
	}
	{
		auto flag = false;
		try {
			parsehex("gg");
		} catch (HexParsingError const&) {
			flag = true;
		}
		assert(flag);
	}

	return 0;
}
