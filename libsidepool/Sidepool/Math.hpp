#pragma once
#ifndef SIDEPOOL_MATH_HPP_
#define SIDEPOOL_MATH_HPP_

#include<cstdint>
#include<memory>

extern "C" {
	struct libsidepool_math;
}

namespace Sidepool {

/** class Sidepool::Math
 *
 * @desc Abstract interface to an SECP256K1
 * library.
 */
class Math {
public:
	virtual ~Math() =default;

	/** Return true if the given
	 * scalar is valid, i.e. 0
	 * to `p - 1`.
	 */
	virtual
	bool
	is_valid(std::uint8_t const scalar[32]) const =0;

	/** Return true if the given
	 * point lies in the SECP256K1
	 * The point at infinity is *not*
	 * on SECP256K1.
	 */
	virtual
	bool
	is_valid_pt(std::uint8_t const point[33]) const =0;

	/** Adds two scalars, modulo `p`.
	 *
	 * The first argument is overwritten
	 * with the sum.
	 */
	virtual
	void add( std::uint8_t scalar_inout[32]
		, std::uint8_t const scalar_inp[32]
		) const =0;

	/** Add two points.
	 *
	 * The first argument is overwritten
	 * with the sum.
	 */
	virtual
	void add_pt( std::uint8_t point_inout[33]
		   , std::uint8_t const point_inp[33]
		   ) const =0;

	/** Negate a scalar.  */
	virtual
	void negate( std::uint8_t scalar_inout[32]
		   ) const =0;

	/** Negate a point. */
	virtual
	void negate_pt( std::uint8_t point_inout[33]
		      ) const =0;

	/** Multiply two scalars.
	 *
	 * The first argument is overwritten
	 * with the product.
	 */
	virtual
	void mul( std::uint8_t scalar_inout[32]
		, std::uint8_t const scalar_in[32]
		) const =0;

	/** Multiply a point by a scalar.
	 *
	 * The first argument is overwritten
	 * with the product.
	 */
	virtual
	void mul_pt( std::uint8_t point_inout[33]
		   , std::uint8_t const scalar_inp[32]
		   ) const =0;

	/** Create from a libsidepool_math*
	 */
	static
	std::unique_ptr<Math>
	create(libsidepool_math*&);
};

}

#endif /* !defined(SIDEPOOL_MATH_HPP_) */
