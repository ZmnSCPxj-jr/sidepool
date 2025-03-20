#if HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/Mod/all.hpp"
#include<vector>

namespace {

class All {
private:
	std::vector<std::shared_ptr<void>> mods;

public:
	All() =default;
	All(All const&) =default;
	All(All&&) =default;
	~All() =default;

	std::shared_ptr<void>
	finalize()&& {
		return std::make_shared<std::vector<std::shared_ptr<void>>>(std::move(mods));
	}

	template<typename Mod, typename... As>
	void add(As&&... as) {
		mods.push_back(
			std::make_shared<Mod>(std::forward(as)...)
		);
	}
};

}

namespace Sidepool::Mod {

std::shared_ptr<void>
all(Sidepool::Util& util) {
	auto rv = All();

	/*
	rv.add<Mod::SomeModule>(util);
	*/

	return std::move(rv).finalize();
}

}
