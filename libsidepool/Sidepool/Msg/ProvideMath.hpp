#pragma once
#ifndef SIDEPOOL_MSG_PROVIDEMATH_HPP_
#define SIDEPOOL_MSG_PROVIDEMATH_HPP_

#include<memory>

namespace Sidepool { class Math; }

namespace Sidepool::Msg {

/** struct Sidepool::Msg::ProvideMath
 *
 * @desc Provides a shared pointer to the math
 * library.
 * This message is broadcast before the library
 * is started.
 */
struct ProvideMath {
	std::shared_ptr<Sidepool::Math> math;
};

}

#endif /* !defined(SIDEPOOL_MSG_PROVIDEMATH_HPP_) */
