#if HAVE_CONFIG_H
# include"config.h"
#endif
#undef NDEBUG
#include"Sidepool/String.hpp"
#include<cassert>
#include<sstream>
#include<vector>


int main() {
	{
		auto vec = std::vector<std::uint8_t>{0x1A, 0xF3, 0x22};
		auto ss = std::ostringstream();
		Sidepool::String::printhex(ss, vec);
		assert(ss.str() == "1AF322");
	}
	{
		auto vec = std::vector<std::uint8_t>{0xF2, 0x19, 0x92, 0xE2};
		auto ss = std::ostringstream();
		Sidepool::String::printhex(ss, vec);
		assert(ss.str() == "F21992E2");
	}
	return 0;
}
