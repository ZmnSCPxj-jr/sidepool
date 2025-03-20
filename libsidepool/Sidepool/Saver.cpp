#if HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/Freed.hpp"
#include"Sidepool/Idler.hpp"
#include"Sidepool/Io.hpp"
#include"Sidepool/Saver.hpp"
#include"libsidepool.h"
#include<cerrno>
#include<exception>

namespace {
typedef Sidepool::Saver::KeyNotFound
	KeyNotFound;
typedef Sidepool::Saver::KeyExists
	KeyExists;
typedef Sidepool::Saver::WrongGeneration
	WrongGeneration;
typedef Sidepool::Saver::IoError
	IoError;
typedef Sidepool::Saver::OutOfSpace
	OutOfSpace;

/* Convert from an errno code to an
std::exception_ptr that we can throw.
*/
void fail_errno( int my_errno
	       , std::function<void(std::exception_ptr)> fail
	       ) {
	if (my_errno == ENOENT) {
		try {
			throw KeyNotFound();
		} catch (...) {
			fail(std::current_exception());
		}
	} else if (my_errno == EEXIST) {
		try {
			throw KeyExists();
		} catch (...) {
			fail(std::current_exception());
		}
	} else if (my_errno == EPERM) {
		try {
			throw WrongGeneration();
		} catch (...) {
			fail(std::current_exception());
		}
	} else if (my_errno == ENOSPC) {
		try {
			throw OutOfSpace();
		} catch (...) {
			fail(std::current_exception());
		}
	} else if (my_errno == ECANCELED) {
		try {
			throw Sidepool::Freed();
		} catch (...) {
			fail(std::current_exception());
		}
	} else {
		/* Treat every other error as EIO.  */
		try {
			throw IoError();
		} catch (...) {
			fail(std::current_exception());
		}
	}
}

class ScanWrapper {
private:
	libsidepool_saver_scan* core;

public:
	ScanWrapper() =delete;
	ScanWrapper(ScanWrapper const&) =delete;

	ScanWrapper(ScanWrapper&& o) {
		core = o.core;
		o.core = nullptr;
	}

	~ScanWrapper() {
		if (core && core->free) {
			core->free(core);
		}
	}

	explicit
	ScanWrapper(libsidepool_saver_scan*& var) {
		core = var;
		var = nullptr;
	}

	std::vector<std::string>
	readall()&& {
		auto rv = std::vector<std::string>();
		for (; core;) {
			auto res = core->get(core);
			if (!res) {
				break;
			}
			rv.emplace_back(res);
		}
		if (core) {
			if (core->free) {
				core->free(core);
			}
			core = nullptr;
		}
		return rv;
	}
};

class SaverWrapper : public Sidepool::Saver {
private:
	typedef std::function<void(std::exception_ptr)>
		FailT;

	libsidepool_saver* core;

	/* Common base which handles `fail`
	 * consistently.
	 */
	class Callback {
	private:
		FailT f_fail;

	public:
		Callback() =delete;
		Callback(Callback&&) =delete;
		Callback(Callback const&) =delete;

		virtual ~Callback() =default;

		explicit
		Callback( FailT f_fail_
			) : f_fail(std::move(f_fail_))
			  { }
		static
		void
		fail( void* context
		    , int my_errno
		    ) {
			auto self = std::unique_ptr<Callback>(static_cast<Callback*>(context));
			auto f_fail = std::move(self->f_fail);
			self = nullptr;
			fail_errno( my_errno
				  , std::move(f_fail)
				  );
		}
	};

	class ScanCallback : public Callback {
	private:
		std::function<void(std::vector<std::string>)> f_pass;
		std::function<void(std::exception_ptr)> f_fail;

	public:
		ScanCallback() =delete;
		ScanCallback(ScanCallback&&) =delete;
		ScanCallback(ScanCallback const&) =delete;

		~ScanCallback() override =default;

		explicit
		ScanCallback( std::function<void(std::vector<std::string>)> f_pass_
			    , FailT f_fail_
			    ) : Callback(std::move(f_fail_))
			      {
			f_pass = std::move(f_pass_);
		}

		/* Passed to C.  */
		static
		void pass( void* context
			 , libsidepool_saver_scan* scan_
			 ) {
			auto self = std::unique_ptr<ScanCallback>(static_cast<ScanCallback*>(context));
			auto scan = ScanWrapper(scan_);

			auto f_pass = std::move(self->f_pass);
			self = nullptr;
			f_pass(std::move(scan).readall());
		}

	};

	class CreateCallback : public Callback {
	private:
		std::function<void(std::uint32_t)> f_pass;

	public:
		CreateCallback() =delete;
		CreateCallback(CreateCallback&&) =delete;
		CreateCallback(CreateCallback const&) =delete;

		~CreateCallback() override =default;

		explicit
		CreateCallback( std::function<void(std::uint32_t)> f_pass_
			      , FailT f_fail_
			      ) : Callback(std::move(f_fail_))
				{
			f_pass = std::move(f_pass_);
		}
		static
		void pass( void* context
			 , uint32_t generation
			 ) {
			auto self = std::unique_ptr<CreateCallback>(static_cast<CreateCallback*>(context));
			auto f_pass = std::move(self->f_pass);
			self = nullptr;
			f_pass(generation);
		}
	};

	class ReadCallback : public Callback {
	private:
		typedef std::pair<std::uint32_t, std::string>
			RetT;
		std::function<void(RetT)> f_pass;

	public:
		ReadCallback() =delete;
		ReadCallback(ReadCallback&&) =delete;
		ReadCallback(ReadCallback const&) =delete;

		~ReadCallback() override =default;

		explicit
		ReadCallback( std::function<void(RetT)> f_pass_
			    , FailT f_fail_
			    ) : Callback(std::move(f_fail_)) {
			f_pass = std::move(f_pass_);
		}
		static
		void pass( void* context
			 , uint32_t generation
			 , char const* value
			 ) {
			auto self = std::unique_ptr<ReadCallback>(static_cast<ReadCallback*>(context));
			auto f_pass = std::move(self->f_pass);
			self = nullptr;
			f_pass(std::make_pair(
				generation,
				std::string(value)
			));
		}
	};

	class UpdateCallback : public Callback {
	private:
		std::function<void(std::uint32_t)> f_pass;

	public:
		UpdateCallback() =delete;
		UpdateCallback(UpdateCallback&&) =delete;
		UpdateCallback(UpdateCallback const&) =delete;

		explicit
		UpdateCallback( std::function<void(std::uint32_t)> f_pass_
			      , FailT f_fail
			      ) : Callback(std::move(f_fail))
				{
			f_pass = std::move(f_pass_);
		}
		static
		void
		pass( void* context
		    , uint32_t generation
		    ) {
			auto self = std::unique_ptr<UpdateCallback>(static_cast<UpdateCallback*>(context));
			auto f_pass = std::move(self->f_pass);
			self = nullptr;
			f_pass(generation);
		}
	};

	class DelCallback : public Callback {
	private:
		std::function<void()> f_pass;

	public:
		DelCallback() =delete;
		DelCallback(DelCallback&&) =delete;
		DelCallback(DelCallback const&) =delete;

		explicit
		DelCallback( std::function<void()> f_pass_
			   , FailT f_fail_
			   ) : Callback(std::move(f_fail_))
			     {
			f_pass = std::move(f_pass_);
		}
		static
		void
		pass(void* context) {
			auto self = std::unique_ptr<DelCallback>(static_cast<DelCallback*>(context));
			auto f_pass = std::move(self->f_pass);
			self = nullptr;
			f_pass();
		}
	};

public:
	SaverWrapper() =delete;
	SaverWrapper(SaverWrapper&&) =delete;
	SaverWrapper(SaverWrapper const&) =delete;

	SaverWrapper( libsidepool_saver*& var
		    ) {
		core = var;
		var = nullptr;
	}

	~SaverWrapper() override {
		if (core->free) {
			core->free(core);
		}
	}

	Sidepool::Io<std::vector<std::string>>
	scan(std::string const& key1) override {
		typedef std::vector<std::string> RetT;
		auto pkey1 = std::make_shared<std::string>(key1);
		return Sidepool::Io<RetT>([ this
					  , pkey1
					  ]( std::function<void(RetT)> pass
					   , std::function<void(std::exception_ptr)> fail
					   ) {
			auto cb = std::make_unique<ScanCallback>(
				std::move(pass),
				std::move(fail)
			);
			core->scan( core
				  , pkey1->c_str()
				  , &ScanCallback::pass
				  , &ScanCallback::fail
				  , (void*) cb.release()
				  );
		});
	}
	Sidepool::Io<std::uint32_t>
	create( std::string const& key1
	      , std::string const& key2
	      , std::string const& value
	      ) override {
		auto pkey1 = std::make_shared<std::string>(key1);
		auto pkey2 = std::make_shared<std::string>(key2);
		auto pvalue = std::make_shared<std::string>(value);
		return Sidepool::Io< std::uint32_t
				   >([ this
				     , pkey1
				     , pkey2
				     , pvalue
				     ]( std::function<void(std::uint32_t)> pass
				      , std::function<void(std::exception_ptr)> fail
				      ) {
			auto cb = std::make_unique<CreateCallback>(
				std::move(pass),
				std::move(fail)
			);
			core->create( core
				    , pkey1->c_str()
				    , pkey2->c_str()
				    , pvalue->c_str()
				    , &CreateCallback::pass
				    , &CreateCallback::fail
				    , (void*) cb.release()
				    );
		});
	}
	Sidepool::Io<std::pair<std::uint32_t, std::string>>
	read( std::string const& key1
	    , std::string const& key2
	    ) override {
		typedef std::pair<std::uint32_t, std::string>
			RetT;
		auto pkey1 = std::make_shared<std::string>(key1);
		auto pkey2 = std::make_shared<std::string>(key2);
		return Sidepool::Io<RetT>([ this
					  , pkey1
					  , pkey2
					  ]( std::function<void(RetT)> pass
					   , std::function<void(std::exception_ptr)> fail
					   ) {
			auto cb = std::make_unique<ReadCallback>(
				std::move(pass),
				std::move(fail)
			);
			core->read( core
				  , pkey1->c_str()
				  , pkey2->c_str()
				  , &ReadCallback::pass
				  , &ReadCallback::fail
				  , (void*) cb.release()
				  );
		});
	}

	Sidepool::Io<std::uint32_t>
	update( std::string const& key1
	      , std::string const& key2
	      , std::uint32_t generation
	      , std::string const& value
	      ) override {
		auto pkey1 = std::make_shared<std::string>(key1);
		auto pkey2 = std::make_shared<std::string>(key2);
		auto pvalue = std::make_shared<std::string>(value);
		return Sidepool::Io<std::uint32_t
				   >([ this
				     , pkey1
				     , pkey2
				     , generation
				     , pvalue
				     ]( std::function<void(std::uint32_t)> pass
				      , FailT fail
				      ) {
			auto cb = std::make_unique<UpdateCallback>(
				std::move(pass),
				std::move(fail)
			);
			core->update( core
				    , pkey1->c_str()
				    , pkey2->c_str()
				    , generation
				    , pvalue->c_str()
				    , &UpdateCallback::pass
				    , &UpdateCallback::fail
				    , (void*) cb.release()
				    );
		});
	}

	Sidepool::Io<void>
	del( std::string const& key1
	   , std::string const& key2
	   , std::uint32_t generation
	   ) override {
		auto pkey1 = std::make_shared<std::string>(key1);
		auto pkey2 = std::make_shared<std::string>(key2);
		return Sidepool::Io<void
				   >([ this
				     , pkey1
				     , pkey2
				     , generation
				     ]( std::function<void()> pass
				      , FailT fail
				      ) {
			auto cb = std::make_unique<DelCallback>(
				std::move(pass),
				std::move(fail)
			);
			core->del( core
				 , pkey1->c_str()
				 , pkey2->c_str()
				 , generation
				 , &DelCallback::pass
				 , &DelCallback::fail
				 , (void*) cb.release()
				 );
		});
	}
};

}

namespace Sidepool {

Sidepool::Io<std::unique_ptr<std::pair<std::uint32_t, std::string>>>
Saver::try_read( std::string const& key1
	       , std::string const& key2
	       ) {
	return read(key1, key2).then([](std::pair<std::uint32_t, std::string> entry) {
		return Sidepool::lift(
			std::make_unique<std::pair<std::uint32_t, std::string>>(std::move(entry))
		);
	}).catching<KeyNotFound>([](KeyNotFound const&) {
		return Sidepool::lift(
			std::unique_ptr<std::pair<std::uint32_t, std::string>>(nullptr)
		);
	});
}

Sidepool::Io<std::string>
Saver::rmw( Sidepool::Idler& idler
	  , std::string const& key1
	  , std::string const& key2
	  , std::function<Sidepool::Io<std::string>(std::string)> func
	  ) {
	typedef std::function<Sidepool::Io<std::string>(std::string)>
		FunType;
	auto pkey1 = std::make_shared<std::string>(key1);
	auto pkey2 = std::make_shared<std::string>(key2);
	auto pfunc = std::make_shared<FunType>(std::move(func));
	return idler.yield().then([ this
				  , pkey1
				  , pkey2
				  ]() {
		return read(*pkey1, *pkey2);
	}).then([ this
		, pkey1
		, pkey2
		, pfunc
		](std::pair<std::uint32_t, std::string> entry) {
		auto generation = entry.first;
		auto value = std::move(entry.second);
		return (*pfunc)(std::move(value)).then([ this
						       , pkey1
						       , pkey2
						       , generation
						       ](std::string nvalue) {
			auto pnvalue = std::make_shared<std::string>(std::move(nvalue));
			return update( *pkey1
				     , *pkey2
				     , generation
				     , *pnvalue
				     ).then([pnvalue](std::uint32_t _generation) {
				return Sidepool::lift(std::make_unique<std::string>(std::move(*pnvalue)));
			}).catching<WrongGeneration>([](WrongGeneration const&) {
				return Sidepool::lift(std::unique_ptr<std::string>(nullptr));
			});
		});
	}).then([ this
		, pkey1
		, pkey2
		, pfunc
		, &idler
		](std::unique_ptr<std::string> result) {
		if (!result) {
			/* Retry.  */
			return rmw( idler
				  , std::move(*pkey1)
				  , std::move(*pkey2)
				  , std::move(*pfunc)
				  );
		}
		return Sidepool::lift(*result);
	});
}
Sidepool::Io<std::unique_ptr<std::string>>
Saver::crud( Sidepool::Idler& idler
	   , std::string const& key1
	   , std::string const& key2
	   , std::function<Sidepool::Io<std::unique_ptr<std::string>>(std::unique_ptr<std::string>)> func
	   ) {
	typedef std::unique_ptr<std::string>
		MayStr;
	typedef std::unique_ptr<MayStr>
		MayMayStr;
	typedef std::pair<std::uint32_t, std::string>
		Entr;
	typedef std::unique_ptr<Entr>
		MayEntr;
	typedef std::function<Sidepool::Io<MayStr>(MayStr)>
		Fun;
	auto pkey1 = std::make_shared<std::string>(key1);
	auto pkey2 = std::make_shared<std::string>(key2);
	auto pfunc = std::make_shared<Fun>(std::move(func));
	return idler.yield().then([ this
				  , pkey1
				  , pkey2
				  ]() {
		return try_read(*pkey1, *pkey2);
	}).then([ this
		, pkey1
		, pkey2
		, pfunc
		](MayEntr entr) {
		auto create_mode = true;
		auto generation = std::uint32_t(0);
		auto value = MayStr(nullptr);
		if (entr) {
			create_mode = false;
			generation = entr->first;
			value = std::make_unique<std::string>(std::move(entr->second));
		}
		return (*pfunc)( std::move(value)
			       ).then([ this
				      , generation
				      , create_mode
				      , pkey1
				      , pkey2
				      , pfunc
				      ](MayStr nvalue) {
			if (create_mode) {
				if (!nvalue) {
					/* Not create
					 * anything, succeed
					 * with the nullptr.
					 */
					return Sidepool::lift(std::make_unique<MayStr>(nullptr));
				}
				auto pnvalue = std::make_shared<std::string>(std::move(*nvalue));
				return create( *pkey1
					     , *pkey2
					     , *pnvalue
					     ).then([ pnvalue
						    ](std::uint32_t _generation) {
					/* Creation succeeded.  */
					return Sidepool::lift(
						std::make_unique<MayStr>(std::make_unique<std::string>(std::move(*pnvalue)))
					);
				}).catching<KeyExists>([](KeyExists const&) {
					/* Creation failed.  */
					return Sidepool::lift(
						MayMayStr(nullptr)
					);
				});
			}
			if (!nvalue) {
				/* Delete the entry.  */
				return del( *pkey1
					  , *pkey2
					  , generation
					  ).then([]() {
					/* Succeed with nullptr.  */
					return Sidepool::lift(
						std::make_unique<MayStr>(nullptr)
					);
				}).catching<KeyNotFound>([](KeyNotFound const&) {
					/* Deletion failed.  */
					return Sidepool::lift(
						MayMayStr(nullptr)
					);
				}).catching<WrongGeneration>([](WrongGeneration const&) {
					/* Deletion failed.  */
					return Sidepool::lift(
						MayMayStr(nullptr)
					);
				});
			}
			/* Update the entry.  */
			auto pnvalue = std::make_shared<std::string>(std::move(*nvalue));
			return update( *pkey1
				     , *pkey2
				     , generation
				     , *pnvalue
				     ).then([pnvalue](std::uint32_t _generation) {
				/* Update succeeded.  */
				return Sidepool::lift(
					std::make_unique<MayStr>(std::make_unique<std::string>(std::move(*pnvalue)))
				);
			}).catching<KeyNotFound>([](KeyNotFound const&) {
				/* Update failed.  */
				return Sidepool::lift(
					MayMayStr(nullptr)
				);
			}).catching<WrongGeneration>([](WrongGeneration const&) {
				/* Update failed.  */
				return Sidepool::lift(
					MayMayStr(nullptr)
				);
			});
		});
	}).then([ this
		, pkey1
		, pkey2
		, pfunc
		, &idler
		](MayMayStr maybe_rv) {
		if (!maybe_rv) {
			/* Need to recurse.  */
			return crud( idler
				   , std::move(*pkey1)
				   , std::move(*pkey2)
				   , std::move(*pfunc)
				   );
		}
		/* Unwrap one layer.  */
		return Sidepool::lift(std::move(*maybe_rv));
	});
}

std::unique_ptr<Saver>
Saver::create(libsidepool_saver*& var) {
	return std::make_unique<SaverWrapper>(var);
}

}
