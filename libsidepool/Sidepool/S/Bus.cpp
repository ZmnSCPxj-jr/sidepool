#if HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/Idler.hpp"
#include"Sidepool/S/Bus.hpp"
#include<map>
#include<memory>

namespace Sidepool::S {

class Bus::Impl {
private:
	std::map< std::type_index
		, std::shared_ptr<Detail::Registry>
		> registries;

public:
	Impl() =default;
	~Impl() =default;

	std::shared_ptr<Detail::Registry>
	get_registry( std::type_index type
		    , std::function<std::shared_ptr<Detail::Registry>()> make_if_absent
		    ) {
		auto it = registries.find(type);
		if (it == registries.end()) {
			if (make_if_absent) {
				auto r = make_if_absent();
				registries[type] = r;
				return r;
			}
			return nullptr;
		}
		return it->second;
	}
};

Bus::Bus() : pimpl(std::make_unique<Impl>()) { }
Bus::~Bus() =default;

std::shared_ptr<Detail::Registry>
Bus::get_registry( std::type_index type
		 , std::function<std::shared_ptr<Detail::Registry>()> make_if_absent
		 ) {
	return pimpl->get_registry(
		type,
		std::move(make_if_absent)
	);
}

}
