#pragma once
#ifndef SIDEPOOL_CRYPTO_SHA256_HPP_
#define SIDEPOOL_CRYPTO_SHA256_HPP_

#include<cstddef>
#include<cstdint>
#include<memory>
#include<string>

namespace Sidepool::Crypto {

/** class Sidepool::Crypto::SHA256
 *
 * @desc A class representing a SHA256
 * midstate.
 */
class SHA256 {
private:
	class Impl;
	std::unique_ptr<Impl> pimpl;

public:
	/** Hash output is fixed 32 bytes.  */
	struct Hash {
		std::uint8_t h[32];

		Hash() =default;
		Hash(Hash const&) =default;
		Hash(std::string const&);

		bool operator!=(Hash const& o) const {
			return !(*this == o);
		}
		bool operator==(Hash const& o) const {
			auto syndrome = std::uint8_t(0);
			for (auto i = 0; i < 32; ++i) {
				syndrome |= h[i] ^ o.h[i];
			}
			return syndrome == 0;
		}
	};

	SHA256();
	SHA256(SHA256 const&);
	SHA256(SHA256&&);
	~SHA256();

	/** Feed some bytes into the hasher.  */
	void feed( std::uint8_t const* dat
		 , std::size_t length
		 );
	/** Feed bytes from an iterator.  */
	template<typename RandIt>
	void feed(RandIt b, RandIt e) {
		feed(&*b, e - b);
	}
	/** Feed bytes from a string.  */
	void feed(std::string const& s) {
		feed( (std::uint8_t const*) &s[0]
		    , s.length()
		    );
	}

	/** Get the hash.  */
	Hash finalize()&&;

	/** Do a single hash run.  */
	static
	Hash hash( std::uint8_t const* data
		 , std::size_t length
		 ) {
		auto state = SHA256();
		state.feed(data, length);
		return std::move(state).finalize();
	}
	template<typename RandIt>
	static
	Hash hash(RandIt b, RandIt e) {
		auto state = SHA256();
		state.feed(b, e);
		return std::move(state).finalize();
	}
	static
	Hash hash(std::string const& s) {
		auto state = SHA256();
		state.feed(s);
		return std::move(state).finalize();
	}
};

}

#endif /* !defined(SIDEPOOL_CRYPTO_SHA256_HPP_) */
