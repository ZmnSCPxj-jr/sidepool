#if HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/Math.hpp"
#include"libsidepool.h"

namespace {

class MathWrapper : public Sidepool::Math {
private:
	libsidepool_math* core;

public:
	MathWrapper() =delete;
	MathWrapper(MathWrapper&&) =delete;
	MathWrapper(MathWrapper const&) =delete;

	MathWrapper(libsidepool_math*& var) {
		core = var;
		var = nullptr;
	}

	~MathWrapper() override {
		if (core->free) {
			core->free(core);
		}
	}

	bool
	is_valid( std::uint8_t const scalar[32]
		) const override {
		return !!core->is_valid(core, scalar);
	}
	bool
	is_valid_pt( std::uint8_t const point[33]
		   ) const override {
		return !!core->is_valid_pt(core, point);
	}
	void
	add( std::uint8_t scalar_inout[32]
	   , std::uint8_t const scalar_inp[32]
	   ) const override {
		core->add( core
			 , scalar_inout
			 , scalar_inp
			 );
	}
	void
	add_pt( std::uint8_t point_inout[33]
	      , std::uint8_t const point_inp[33]
	      ) const override {
		core->add_pt( core
			    , point_inout
			    , point_inp
			    );
	}
	void negate( std::uint8_t scalar_inout[32]
		   ) const override {
		core->negate( core
			    , scalar_inout
			    );
	}
	void negate_pt( std::uint8_t point_inout[33]
		      ) const override {
		core->negate_pt( core
			       , point_inout
			       );
	}
	void mul( std::uint8_t scalar_inout[32]
		, std::uint8_t const scalar_inp[32]
		) const override {
		core->mul( core
			 , scalar_inout
			 , scalar_inp
			 );
	}
	void mul_pt( std::uint8_t point_inout[33]
		   , std::uint8_t const scalar_inp[32]
		   ) const override {
		core->mul_pt( core
			    , point_inout
			    , scalar_inp
			    );
	}
};

}

namespace Sidepool {

std::unique_ptr<Math>
create(libsidepool_math*& var) {
	return std::make_unique<MathWrapper>(var);
}

}
