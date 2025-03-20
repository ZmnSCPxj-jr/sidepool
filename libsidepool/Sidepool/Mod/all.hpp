#pragma once
#ifndef SIDEPOOL_MOD_ALL_HPP_
#define SIDEPOOL_MOD_ALL_HPP_

#include<memory>

namespace Sidepool { class Util; }

namespace Sidepool::Mod {

/** Sidepool::Mod::all
 *
 * @desc Constructs all the modules, then
 * returns a convenient smart pointer that
 * can be dropped to deconstruct all of
 * them.
 */
std::shared_ptr<void> all(Sidepool::Util&);

}

#endif /* !defined(SIDEPOOL_MOD_ALL_HPP_) */
