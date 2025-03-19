#pragma once
#ifndef TESTING_IDLER_HPP_
#define TESTING_IDLER_HPP_

#include"Sidepool/Idler.hpp"
#include"Sidepool/Freed.hpp"
#include<exception>
#include<queue>
#include<utility>

namespace Testing {

class Idler : public Sidepool::Idler {
private:
	struct Item {
		std::function<void()> pass;
		std::function<void(std::exception_ptr)> fail;
	};
	std::queue<Item> items;

	void add_item( std::function<void()> pass
		     , std::function<void(std::exception_ptr)> fail
		     ) {
		items.push(Item{
			std::move(pass),
			std::move(fail)
		});
	}

public:
	/* Movable, but not copyable.  */
	Idler() =default;
	Idler(Idler&&) =default;

	Idler(Idler const&) =delete;

	/* Throw `Sidepool::Freed` on everything.  */
	~Idler() {
		try {
			throw Sidepool::Freed();
		} catch (...) {
			auto e = std::current_exception();
			while (!items.empty()) {
				auto& front = items.front();
				auto fail = std::move(
					front.fail
				);
				front.pass = nullptr;
				items.pop();
				/* front is now a dangling
				reference!
				*/

				fail(e);
			}
		}
	}

	/* Preferred constructor.  */
	static
	std::unique_ptr<Idler> make() {
		return std::make_unique<Idler>();
	}

	/* Function to keep running everything
	until everything has completed.

	In the testing environment, we generally
	do not actually go block on anything,
	since everything that could potentially
	block would actually just be mocked
	testing models.
	*/
	void run() {
		while (!items.empty()) {
			auto& front = items.front();
			auto pass = std::move(
				front.pass
			);
			front.fail = nullptr;
			items.pop();
			pass();
		}
	}

	void start(Sidepool::Io<void> io) override {
		auto iop = std::make_shared<Sidepool::Io<void>>(std::move(io));
		auto ignore_fail = [](std::exception_ptr) {
		};
		auto start = [iop]() {
			auto my_pass = []() { };
			auto my_fail = [](std::exception_ptr) { };
			iop->run( std::move(my_pass)
				, std::move(my_fail)
				);
		};
		add_item( std::move(start)
			, std::move(ignore_fail)
			);
	}
	Sidepool::Io<void>
	yield() override {
		return Sidepool::Io<void>([ this
					  ]( std::function<void()> pass
					  , std::function<void(std::exception_ptr)> fail
					  ) {
			add_item( std::move(pass)
				, std::move(fail)
				);
		});
	}
	Sidepool::Io<void>
	fork(Sidepool::Io<void> io) override {
		auto iop = std::make_shared<Sidepool::Io<void>>(std::move(io));
		return Sidepool::lift().then([this, iop]() {
			start(std::move(*iop));
			return yield();
		});
	}
};

}

#endif /* !defined(TESTING_IDLER_HPP_) */
