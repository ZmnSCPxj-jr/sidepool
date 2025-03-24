#pragma once
#ifndef SIDEPOOL_M_HPP_
#define SIDEPOOL_M_HPP_

#include<cstdint>
#include<iostream>
#include<memory>
#include<stdexcept>

namespace Sidepool { class Math; }

namespace Sidepool::M {

struct Vec32 {
	std::uint8_t bytes[32];
};

struct InvalidScalar : public std::runtime_error {
	InvalidScalar() : std::runtime_error("Invalid scalar") { }
};

/** class Sidepool::M::Scalar
 *
 * @desc Represents an SECP256K1 scalar.
 */
class Scalar {
private:
	class Impl;
	std::unique_ptr<Impl> pimpl;

public:
	Scalar() =delete;

	explicit
	Scalar(std::shared_ptr<Math> math);
	Scalar(Scalar&&);
	Scalar(Scalar const&);
	~Scalar();

	explicit
	Scalar( std::shared_ptr<Math>
	      , Vec32 const&
	      );
	explicit
	Scalar( std::shared_ptr<Math>
	      , std::string const&
	      );

	Vec32 const& vec32() const;

	Scalar& operator=(Scalar const&);
	Scalar& operator=(Scalar&&);
	Scalar& operator+=(Scalar const& o);
	Scalar& negate();
	Scalar& operator*=(Scalar const& o);
	bool operator==(Scalar const& o) const;
	operator bool() const;

	Scalar operator+(Scalar const& o) const {
		auto tmp = Scalar(*this);
		tmp += o;
		return tmp;
	}
	Scalar operator-() const {
		auto tmp = Scalar(*this);
		tmp.negate();
		return tmp;
	}
	Scalar& operator-=(Scalar const& o) {
		return (*this) += (-o);
	}
	Scalar operator-(Scalar const& o) const {
		auto tmp = -o;
		tmp += *this;
		return tmp;
	}
	Scalar operator*(Scalar const& o) const {
		auto tmp = Scalar(*this);
		tmp *= o;
		return tmp;
	}

	bool operator!=(Scalar const& o) const {
		return !(*this == o);
	}
	bool operator!() const {
		return !((bool) *this);
	}
};

std::ostream& operator<<(std::ostream&, Scalar const&);

struct Vec33 {
	std::uint8_t bytes[33];
};

struct InvalidPoint : public std::runtime_error {
	InvalidPoint() : std::runtime_error("Invalid point") { }
};

/** class Sidepool::M::Point
 *
 * @desc Represents an SECP256K1 point.
 */
class Point {
private:
	class Impl;
	std::unique_ptr<Impl> pimpl;

public:
	Point() =delete;

	/* Constructs standard G.  */
	explicit
	Point(std::shared_ptr<Math> math);
	Point(Point&&);
	Point(Point const&);
	~Point();

	explicit
	Point( std::shared_ptr<Math>
	     , Vec33 const&
	     );
	explicit
	Point( std::shared_ptr<Math>
	     , std::string const&
	     );

	Vec33 const& vec33() const;

	Point& operator=(Point const&);
	Point& operator=(Point&&);
	Point& operator+=(Point const&);
	Point& negate();
	Point& operator*=(Scalar const& o);
	bool operator==(Point const& o) const;

	Point operator+(Point const& o) const {
		auto tmp = Point(*this);
		tmp += o;
		return tmp;
	}
	Point operator-() const {
		auto tmp = Point(*this);
		tmp.negate();
		return tmp;
	}
	Point& operator-=(Point const& o) {
		return (*this) += (-o);
	}
	Point operator-(Point const& o) const {
		auto tmp = -o;
		tmp += *this;
		return tmp;
	}
	Point operator*(Scalar const& o) const {
		auto tmp = *this;
		tmp *= o;
		return tmp;
	}

	bool operator!=(Point const& o) const {
		return !(*this == o);
	}
};

inline
Point operator*(Scalar const& s, Point const& p) {
	return p * s;
}

std::ostream& operator<<(std::ostream&, Point const&);

}

#endif /* !defined(SIDEPOOL_M_HPP_) */
