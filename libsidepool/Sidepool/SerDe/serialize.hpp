#pragma once
#ifndef SIDEPOOL_SERDE_SERIALIZE_HPP_
#define SIDEPOOL_SERDE_SERIALIZE_HPP_

#include"Sidepool/SerDe/Trait.hpp"

namespace Sidepool::SerDe {

template<typename T>
void serialize( Archive& a
	      , T& obj
	      ) {
	Trait<T>::serialize(a, obj);
}

}

#endif /* !defined(SIDEPOOL_SERDE_SERIALIZE_HPP_) */
