#pragma once
#ifndef SIDEPOOL_MSG_PROVIDESAVER_HPP_
#define SIDEPOOL_MSG_PROVIDESAVER_HPP_

namespace Sidepool { class Saver; }

namespace Sidepool::Msg {

/** struct Sidepool::Msg::ProvideSaver;
 *
 * @desc Message used to provide the
 * instance of the `Sidepool::Saver`
 * in use.
 */
struct ProvideSaver {
	Sidepool::Saver& saver;
};

}

#endif /* !defined(SIDEPOOL_MSG_PROVIDESAVER_HPP_) */
