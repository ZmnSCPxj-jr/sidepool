#pragma once
#ifndef SIDEPOOL_BASE64X_HPP_
#define SIDEPOOL_BASE64X_HPP_

#include<cstdint>
#include<stdexcept>
#include<string>
#include<vector>

namespace Sidepool::Base64x {

/** Sidepool::Base64x::encode
 *
 * @desc Encode the given byte vector into a variant of
 * base64 where we use `%` instead of `/`.
 *
 * We use `%` because we promise in `libsidepool.h` that
 * strings put in the saver will never contain `/`, but
 * could contain `%`.
 */
std::string encode(std::vector<std::uint8_t> const&);

/** Sidepool::Base64x::decode
 *
 * @desc Decode a string generated by `encode` above
 * into a byte vector.
 */
std::vector<std::uint8_t> decode(std::string const&);

/** struct Sidepool::Base64x::DecodeError
 *
 * @desc thrown by `Sidepool::Base64x::decode` if the
 * given string has an invalid length or character.
 */
struct DecodeError : public std::runtime_error {
	DecodeError() : std::runtime_error("Base64x decoding error") { }
};

}

#endif /* !defined(SIDEPOOL_BASE64X_HPP_) */
