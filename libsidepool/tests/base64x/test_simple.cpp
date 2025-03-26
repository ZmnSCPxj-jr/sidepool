#if HAVE_CONFIG_H
# include"config.h"
#endif
#undef NDEBUG
#include"Sidepool/Base64x.hpp"
#include<cassert>

#include<iostream>

int main() {
	using Sidepool::Base64x::DecodeError;
	using Sidepool::Base64x::decode;
	using Sidepool::Base64x::encode;

	{
		/* Empty vector should encode to empty string.  */
		assert(encode(std::vector<std::uint8_t>{}) == "");
	}
	{
		/* Empty string should decode to empty vector.  */
		auto d = decode("");
		assert(d.size() == 0);
	}
	/* Comprehensive checks.  */
	{
		auto vec = std::vector<std::uint8_t>{
			0x01
		};
		auto b = encode(vec);
		auto dec = decode(b);
		assert(vec == dec);
	}
	{
		auto vec = std::vector<std::uint8_t>{
			0x05, 0xFF
		};
		auto b = encode(vec);
		auto dec = decode(b);
		assert(vec == dec);
	}
	{
		auto vec = std::vector<std::uint8_t>{
			0x42, 0x99, 0xEF
		};
		auto b = encode(vec);
		auto dec = decode(b);
		assert(vec == dec);
	}
	{
		auto vec = std::vector<std::uint8_t>{
			0xEF, 0x23, 0x5B, 0x0D
		};
		auto b = encode(vec);
		auto dec = decode(b);
		assert(vec == dec);
	}

	/* Check decode errors.  */
	{
		auto flag = false;
		try {
			(void) decode("====");
			flag = false;
		} catch (DecodeError const& _) {
			flag = true;
		}
		assert(flag);
	}
	{
		auto flag = false;
		try {
			(void) decode("A===");
			flag = false;
		} catch (DecodeError const& _) {
			flag = true;
		}
		assert(flag);
	}
	{
		auto flag = false;
		try {
			(void) decode("////");
			flag = false;
		} catch (DecodeError const& _) {
			flag = true;
		}
		assert(flag);
	}
	{
		auto flag = false;
		try {
			(void) decode("ABC");
			flag = false;
		} catch (DecodeError const& _) {
			flag = true;
		}
		assert(flag);
	}

	return 0;
}
