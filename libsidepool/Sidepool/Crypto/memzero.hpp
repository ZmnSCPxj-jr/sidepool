#pragma once
#ifndef SIDEPOOL_CRYPTO_MEMZERO_HPP_
#define SIDEPOOL_CRYPTO_MEMZERO_HPP_

#include<cstddef>

namespace Sidepool::Crypto {

/** Sidepool::Crypto::memzero
 *
 * @desc Function that securely clears the
 * memory area in such a way that it is
 * not removed by compiler optimizations.
 */
void memzero(void* dat, std::size_t len);

}

#endif /* !defined(SIDEPOOL_CRYPTO_MEMZERO_HPP_) */
