#if HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/M.hpp"
#include"Sidepool/Math.hpp"
#include"Sidepool/String.hpp"
#include<cassert>
#include<cstring>
#include<utility>

namespace Sidepool::M {

class Scalar::Impl {
private:
	std::shared_ptr<Math> math;
	Vec32 vec;

	void validate() {
		if (!math->is_valid(vec.bytes)) {
			throw InvalidScalar();
		}
	}

public:
	Impl() =delete;
	explicit
	Impl(std::shared_ptr<Math> math_) : math(std::move(math_)) {
		std::memset(vec.bytes, 0, 32);
	}
	Impl(Impl&&) =delete;
	Impl(Impl const& o
	    ) : math(o.math)
	      , vec(o.vec)
	      { validate(); }

	explicit
	Impl( std::shared_ptr<Math> math_
	    , Vec32 const& vec_
	    ) : math(std::move(math_))
	      , vec(vec_)
	      { validate(); }

	explicit
	Impl( std::shared_ptr<Math> math_
	    , std::string const& hex
	    ) : math(std::move(math_))
	      {
		auto dat = Sidepool::String::parsehex(hex);
		if (dat.size() != 32) {
			throw Sidepool::String::HexParsingError();
		}
		std::memcpy(vec.bytes, &dat[0], 32);
		validate();
	}

	Vec32 const& vec32() const { return vec; }

	Impl& operator=(Impl const&) =default;
	void add(Impl const& o) {
		math->add(vec.bytes, o.vec.bytes);
	}
	void negate() {
		math->negate(vec.bytes);
	}
	void mul(Impl const& o) {
		math->mul(vec.bytes, o.vec.bytes);
	}
	bool eq(Impl const& o) const {
		auto syndrome = std::uint8_t(0);
		for (auto i = 0; i < 32; ++i) {
			syndrome |= (vec.bytes[i] ^ o.vec.bytes[i]);
		}
		return (syndrome == 0);
	}
	bool isnonzero() const {
		auto syndrome = std::uint8_t(0);
		for (auto i = 0; i < 32; ++i) {
			syndrome |= vec.bytes[i];
		}
		return (syndrome != 0);
	}
};

Scalar::Scalar(Scalar&& o) {
	if (o.pimpl) {
		pimpl = std::move(o.pimpl);
	}
}
Scalar::Scalar(Scalar const& o
	      ) {
	if (o.pimpl) {
		pimpl = std::make_unique<Impl>(*o.pimpl);
	}
}

Scalar::Scalar( std::shared_ptr<Math> math
	      ) : pimpl(std::make_unique<Impl>(std::move(math))) { }
Scalar::Scalar( std::shared_ptr<Math> math
	      , Vec32 const& v32
	      ) : pimpl(std::make_unique<Impl>( std::move(math)
					      , v32
					      )) { }

Scalar::~Scalar() =default;

Vec32 const&
Scalar::vec32() const {
	assert(pimpl);
	return pimpl->vec32();
}

Scalar&
Scalar::operator=(Scalar const& o) {
	if (!pimpl) {
		if (o.pimpl) {
			pimpl = std::make_unique<Impl>(*o.pimpl);
		}
	} else if (o.pimpl) {
		*pimpl = *o.pimpl;
	} else {
		pimpl = nullptr;
	}
	return *this;
}
Scalar&
Scalar::operator=(Scalar&& o) {
	auto tmp = Scalar(std::move(o));
	std::swap(pimpl, tmp.pimpl);
	return *this;
}
Scalar&
Scalar::operator+=(Scalar const& o) {
	assert(pimpl);
	assert(o.pimpl);
	pimpl->add(*o.pimpl);
	return *this;
}
Scalar&
Scalar::negate() {
	assert(pimpl);
	pimpl->negate();
	return *this;
}
Scalar&
Scalar::operator*=(Scalar const& o) {
	assert(pimpl);
	assert(o.pimpl);
	pimpl->mul(*o.pimpl);
	return *this;
}
bool
Scalar::operator==(Scalar const& o) const {
	assert(pimpl);
	assert(o.pimpl);
	return pimpl->eq(*o.pimpl);
}
Scalar::operator bool() const {
	assert(pimpl);
	return pimpl->isnonzero();
}

std::ostream& operator<<(std::ostream& os, Scalar const& s) {
	auto const& v32 = s.vec32();
	Sidepool::String::printhex(os, v32.bytes, v32.bytes + 32);
	return os;
}

namespace {

std::uint8_t const g[33] = {
	0x02,
	0x79, 0xBE, 0x66, 0x7E,
	0xF9, 0xDC, 0xBB, 0xAC,
	0x55, 0xA0, 0x62, 0x95,
	0xCE, 0x87, 0x0B, 0x07,
	0x02, 0x9B, 0xFC, 0xDB,
	0x2D, 0xCE, 0x28, 0xD9,
	0x59, 0xF2, 0x81, 0x5B,
	0x16, 0xF8, 0x17, 0x98
};

}

class Point::Impl {
private:
	std::shared_ptr<Math> math;
	Vec33 vec;

	void validate() {
		if (!math->is_valid_pt(vec.bytes)) {
			throw InvalidPoint();
		}
	}

public:
	Impl() =delete;
	Impl(Impl&&) =delete;

	Impl(Impl const& o
	    ) : math(o.math)
	      , vec(o.vec)
	      { validate(); }

	Impl(std::shared_ptr<Math> math_) : math(std::move(math_)) {
		std::memcpy(vec.bytes, g, 33);
	}

	Impl( std::shared_ptr<Math> math_
	    , Vec33 const& vec_
	    ) : math(std::move(math_))
	      , vec(vec_)
	      { validate(); }
	Impl( std::shared_ptr<Math> math_
	    , std::string const& hex
	    ) : math(std::move(math_)) {
		auto dat = Sidepool::String::parsehex(hex);
		if (dat.size() != 33) {
			throw Sidepool::String::HexParsingError();
		}
		std::memcpy(vec.bytes, &dat[0], 33);
		validate();
	}

	Vec33 const& vec33() const { return vec; }

	Impl& operator=(Impl const&) =default;
	void add(Impl const& o) {
		math->add_pt(vec.bytes, o.vec.bytes);
	}
	void negate() {
		math->negate_pt(vec.bytes);
	}
	void mul(Scalar const& s) {
		auto const& sv = s.vec32();
		math->mul_pt(vec.bytes, sv.bytes);
	}
	bool eq(Impl const& o) {
		auto syndrome = std::uint8_t(0);
		for (auto i = 0; i < 33; ++i) {
			syndrome |= (vec.bytes[i] ^ o.vec.bytes[i]);
		}
		return (syndrome == 0);
	}
};

Point::Point( std::shared_ptr<Math> math
	    ) : pimpl(std::make_unique<Impl>(std::move(math))) { }

Point::Point(Point&& o) {
	if (o.pimpl) {
		pimpl = std::move(o.pimpl);
	}
}
Point::Point(Point const& o) {
	if (o.pimpl) {
		pimpl = std::make_unique<Impl>(*o.pimpl);
	}
}
Point::~Point() =default;

Point::Point( std::shared_ptr<Math> math
	    , Vec33 const& v33
	    ) : pimpl(std::make_unique<Impl>( std::move(math)
					    , v33
					    )) { }
Point::Point( std::shared_ptr<Math> math
	    , std::string const& s
	    ) : pimpl(std::make_unique<Impl>( std::move(math)
					    , s
					    )) { }

Vec33 const&
Point::vec33() const {
	assert(pimpl);
	return pimpl->vec33();
}

Point&
Point::operator=(Point const& o) {
	if (!pimpl) {
		if (o.pimpl) {
			pimpl = std::make_unique<Impl>(*o.pimpl);
		}
	} else if (o.pimpl) {
		*pimpl = *o.pimpl;
	} else {
		pimpl = nullptr;
	}
	return *this;
}
Point&
Point::operator=(Point&& o) {
	auto tmp = Point(std::move(o));
	std::swap(pimpl, tmp.pimpl);
	return *this;
}

Point&
Point::operator+=(Point const& o) {
	assert(pimpl);
	assert(o.pimpl);
	pimpl->add(*o.pimpl);
	return *this;
}
Point&
Point::negate() {
	assert(pimpl);
	pimpl->negate();
	return *this;
}
Point&
Point::operator*=(Scalar const& o) {
	assert(pimpl);
	pimpl->mul(o);
	return *this;
}
bool
Point::operator==(Point const& o) const {
	assert(pimpl);
	assert(o.pimpl);
	return pimpl->eq(*o.pimpl);
}

std::ostream& operator<<(std::ostream& os, Point const& p) {
	auto const& v33 = p.vec33();
	Sidepool::String::printhex(os, v33.bytes, v33.bytes + 33);
	return os;
}

}
