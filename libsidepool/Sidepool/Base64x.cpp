#if HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/Base64x.hpp"
#include<cassert>
#include<sstream>

namespace {

/* Return the character equivalent to the given 6-bit
number.
*/
char encode_b64(std::uint8_t b) {
	assert(b <= 63);
	if (b < 26) {
		return 'A' + char(b);
	} else if (b < 52) {
		return 'a' + char(b - 26);
	} else if (b < 61) {
		return '0' + char(b - 52);
	} else if (b == 62) {
		return '+';
	} else {
		return '%';
	}
}
/* Return the 6-bit number equivalent to the character, or
throw DecodeError if invalid character.
*/
std::uint8_t decode_b64(char c) {
	if ('A' <= c && c <= 'Z') {
		return std::uint8_t(c - 'A');
	} else if ('a' <= c && c <= 'z') {
		return std::uint8_t(c - 'a') + 26;
	} else if ('0' <= c && c <= '9') {
		return std::uint8_t(c - '0') + 52;
	} else if (c == '+') {
		return 62;
	} else if (c == '%') {
		return 63;
	} else {
		throw Sidepool::Base64x::DecodeError();
	}
}

class Encoder {
private:
	int step;
	std::uint8_t bits;
	std::ostringstream os;

public:
	Encoder() {
		step = 0;
		bits = 0;
	}

	void feed(std::uint64_t b) {
		if (step == 0) {
			bits = b & 0x3;
			auto cur = b >> 2;
			os.put(encode_b64(cur));
		} else if (step == 1) {
			auto cur = (bits << 4) | (b >> 4);
			os.put(encode_b64(cur));
			bits = b & 0xF;
		} else {
			auto cur = (bits << 2) | (b >> 6);
			os.put(encode_b64(cur));
			auto last = b & 0x3F;
			os.put(encode_b64(last));
			bits = 0;
		}
		step = (step + 1) % 3;
	}

	std::string finish()&& {
		if (step != 0) {
			if (step == 1) {
				os.put(encode_b64(bits << 4));
				os << "==";
			} else if (step == 2) {
				os.put(encode_b64(bits << 2));
				os << "=";
			}
		}
		return os.str();
	}
};

class Decoder {
private:
	std::vector<std::uint8_t> ov;
	int step;
	std::uint8_t bits;

public:
	Decoder() {
		step = 0;
		bits = 0;
	}

	void feed(char c) {
		if (step < 0) {
			if (c != '=') {
				throw Sidepool::Base64x::DecodeError();
			}
			return;
		}
		if (step == 0) {
			bits = decode_b64(c);
		} else if (step == 1) {
			auto cur = decode_b64(c);
			auto byt = (bits << 2) | (cur >> 4);
			ov.push_back(byt);
			bits = cur & 0xF;
		} else if (step == 2) {
			if (c == '=') {
				step = -1;
				return;
			}
			auto cur = decode_b64(c);
			auto byt = (bits << 4) | (cur >> 2);
			ov.push_back(byt);
			bits = cur & 0x3;
		} else {
			if (c == '=') {
				step = -1;
				return;
			}
			auto cur = decode_b64(c);
			auto byt = (bits << 6) | cur;
			ov.push_back(byt);
			bits = 0;
		}
		step = (step + 1) % 4;
	}

	std::vector<std::uint8_t> finish()&& {
		if (step != 0 && step >= 0) {
			throw Sidepool::Base64x::DecodeError();
		}
		return std::move(ov);
	}
};

}

namespace Sidepool::Base64x {

std::string
encode(std::vector<std::uint8_t> const& v) {
	auto enc = Encoder();
	for (auto p = v.begin(); p != v.end(); ++p) {
		enc.feed(*p);
	}
	return std::move(enc).finish();
}

std::vector<std::uint8_t>
decode(std::string const& s) {
	auto dec = Decoder();
	for (auto p = s.begin(); p != s.end(); ++p) {
		dec.feed(*p);
	}
	return std::move(dec).finish();
}

}
