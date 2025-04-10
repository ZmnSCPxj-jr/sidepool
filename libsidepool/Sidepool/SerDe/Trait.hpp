#pragma once
#ifndef SIDEPOOL_SERDE_TRAIT_HPP_
#define SIDEPOOL_SERDE_TRAIT_HPP_

#include"Sidepool/SerDe/Archive.hpp"

#include<cstdint>
#include<memory>
#include<string>
#include<vector>

namespace Sidepool::SerDe {

/** struct Sidepool::SerDe::Trait<T>
 *
 * @desc A trait struct which can be extended by
 * other modules.
 *
 * Every trait instance must have a static member
 * function:
 *
 * `static void serialize(Archive& a, T& obj);`
 *
 * This member function serializes the object to
 * the archive.
 *
 * Actual users of the `SerDe` system should
 * call `Sidepool::SerDe::serialize`.
 * Only use this trait to override it for your
 * specific type.
 */
template<typename T>
struct Trait;

template<>
struct Trait<bool> {
	static void serialize(Archive& a, bool& b) {
		auto byt = b ? std::uint8_t(1) : std::uint8_t(0);
		a.ser_byte(byt);
		b = !!byt;
	}
};

template<>
struct Trait<char> {
	static void serialize(Archive& a, char& c) {
		auto b = std::uint8_t(c);
		a.ser_byte(b);
		c = char(b);
	}
};
template<>
struct Trait<std::uint8_t> {
	static
	void serialize( Archive& a
		      , std::uint8_t& b
		      ) {
		a.ser_byte(b);
	}
};
template<>
struct Trait<std::int8_t> {
	static
	void serialize( Archive& a
		      , std::int8_t& ib
		      ) {
		auto ub = std::uint8_t(ib);
		a.ser_byte(ub);
		ib = std::int8_t(ub);
	}
};

template<>
struct Trait<std::uint16_t> {
	static
	void serialize( Archive& a
		      , std::uint16_t& w
		      ) {
		auto b0 = std::uint8_t((w >> 8) & 0xFF);
		auto b1 = std::uint8_t((w >> 0) & 0xFF);
		Trait<std::uint8_t>::serialize(a, b0);
		Trait<std::uint8_t>::serialize(a, b1);
		w = (std::uint16_t(b0) << 8)
		  | (std::uint16_t(b1) << 0)
		  ;
	}
};
template<>
struct Trait<std::int16_t> {
	static
	void serialize( Archive& a
		      , std::int16_t& iw
		      ) {
		auto uw = std::uint16_t(iw);
		Trait<std::uint16_t>::serialize(a, uw);
		iw = std::int16_t(uw);
	}
};

template<>
struct Trait<std::uint32_t> {
	static
	void serialize( Archive& a
		      , std::uint32_t& w
		      ) {
		auto b0 = std::uint16_t((w >> 16) & 0xFFFF);
		auto b1 = std::uint16_t((w >> 0) & 0xFFFF);
		Trait<std::uint16_t>::serialize(a, b0);
		Trait<std::uint16_t>::serialize(a, b1);
		w = (std::uint32_t(b0) << 16)
		  | (std::uint32_t(b1) << 0)
		  ;
	}
};
template<>
struct Trait<std::int32_t> {
	static
	void serialize( Archive& a
		      , std::int32_t& iw
		      ) {
		auto uw = std::uint32_t(iw);
		Trait<std::uint32_t>::serialize(a, uw);
		iw = std::int32_t(uw);
	}
};

template<>
struct Trait<std::uint64_t> {
	static
	void serialize( Archive& a
		      , std::uint64_t& w
		      ) {
		auto b0 = std::uint32_t((w >> 32) & 0xFFFFFFFF);
		auto b1 = std::uint32_t((w >> 0) & 0xFFFFFFFF);
		Trait<std::uint32_t>::serialize(a, b0);
		Trait<std::uint32_t>::serialize(a, b1);
		w = (std::uint64_t(b0) << 32)
		  | (std::uint64_t(b1) << 0)
		  ;
	}
};
template<>
struct Trait<std::int64_t> {
	static
	void serialize( Archive& a
		      , std::int64_t& iw
		      ) {
		auto uw = std::uint64_t(iw);
		Trait<std::uint64_t>::serialize(a, uw);
		iw = std::int64_t(uw);
	}
};

template<>
struct Trait<std::string> {
	static
	void serialize( Archive& a
		      , std::string& s
		      ) {
		auto len = std::uint64_t(s.size());
		Trait<std::uint64_t>::serialize(a, len);
		if (std::size_t(len) != s.size()) {
			s.resize(std::size_t(len));
		}
		for (auto& c : s) {
			Trait<char>::serialize(a, c);
		}
	}
};

template<typename T>
struct Trait<std::vector<T>> {
	static
	void serialize( Archive& a
		      , std::vector<T>& v
		      ) {
		auto len = std::uint64_t(v.size());
		Trait<std::uint64_t>::serialize(a, len);
		if (std::size_t(len) != v.size()) {
			v.resize(std::size_t(len));
		}
		for (auto& i : v) {
			Trait<T>::serialize(a, i);
		}
	}
};

template<typename T>
struct Trait<std::unique_ptr<T>> {
	static
	void serialize( Archive& a
		      , std::unique_ptr<T>& p
		      ) {
		auto exist = bool(p);
		Trait<bool>::serialize(a, exist);
		if (exist) {
			if (!p) {
				p = std::make_unique<T>();
			}
			Trait<T>::serialize(a, *p);
		} else {
			p = nullptr;
		}
	}
};
/* Not safe to use std::shared_ptr, as objects it
refers to may be shared outside of the object being
serialized.
*/

}

#endif /* !defined(SIDEPOOL_SERDE_TRAIT_HPP_) */
