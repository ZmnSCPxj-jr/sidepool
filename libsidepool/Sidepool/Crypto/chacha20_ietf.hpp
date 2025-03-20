#pragma once
#ifndef SIDEPOOL_CRYPTO_CHACHA20_IETF_HPP_
#define SIDEPOOL_CRYPTO_CHACHA20_IETF_HPP_

#include<cstddef>
#include<cstdint>

namespace Sidepool::Crypto {

/** Sidepool::Crypto::chacha20_ietf
 *
 * @desc ChaCha20 stream cipher implementation.
 */
void
chacha20_ietf( std::uint8_t* out
	     , std::size_t out_len
	     , std::uint8_t key[32]
	     , std::uint8_t nonce[12]
	     );

}

#endif /* !defined(SIDEPOOL_CRYPTO_CHACHA20_IETF_HPP_) */
