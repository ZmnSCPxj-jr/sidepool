#pragma once
#ifndef SIDEPOOL_SAVER_HPP_
#define SIDEPOOL_SAVER_HPP_

#include<cstdint>
#include<exception>
#include<functional>
#include<memory>
#include<string>
#include<utility>
#include<vector>

extern "C" {
	struct libsidepool_saver;
}

namespace Sidepool { class Idler; }
namespace Sidepool { template<typename a> class Io; }

namespace Sidepool {

/** class Sidepool::Saver
 *
 * @desc Abstract interface to an object
 * that can handle persistent storage.
 *
 * The model is that there are two
 * hierarchical keyspaces, `key1` and
 * `key2`, both strings.
 * A pair `key1,key2` represents a
 * combined key, associated with some
 * string value.
 *
 * In addition, the saver can be given
 * `key1` and it would list all `key2`s
 * where `key1,key2` has an associated
 * string value.
 *
 * In addition to a string value, a
 * numeric unsigned 32-bit "generation"
 * is associated with each value.
 * When reading an entry, the saver
 * also returns the generation.
 * When overwriting an entry, the
 * saver requires that you give the
 * expected generation, and atomically
 * checks the actual on-disk generation
 * versus the expected one, and only
 * actual overwrites if they match,
 * then increments the generation.
 */
class Saver {
public:
	virtual ~Saver() { }

	/* Various errors thrown.

	The operations may also throw
	Sidepool::Freed if the saver
	is freed before it can return
	the result.
	*/
	/** Thrown if you ask to read,
	ovewrite, or delete a `key1,key2`
	that does not exist.
	*/
	struct KeyNotFound : public std::runtime_error {
		KeyNotFound()
			: std::runtime_error("key not found") { }
	};
	/** Thrown if you are trying to
	 * create a new entry but it
	 * already exists.
	 */
	struct KeyExists : public std::runtime_error {
		KeyExists()
			: std::runtime_error("key already exists") { }
	};
	/** Thrown if you are trying to
	 * overwrite or delete an entry
	 * but the expected generation
	 * is not matched.
	 */
	struct WrongGeneration : public std::runtime_error {
		WrongGeneration()
			: std::runtime_error("unexpected generation") { }
	};
	/** Thrown if there is a read or
	 * or write error.
	 */
	struct IoError : public std::runtime_error {
		IoError()
			: std::runtime_error("I/O error") {}
	};
	/** Thrown if there is an
	 * out-of-space error.
	 */
	struct OutOfSpace : public std::runtime_error {
		OutOfSpace()
			: std::runtime_error("no space left") {}
	};

	/** Given a `key1` return a list of
	 * `key2`s where `key1,key2` has an
	 * associated generation and value.
	 *
	 * Throws:
	 * - `Sidepool::Freed`
	 * - `IoError`
	 */
	virtual
	Sidepool::Io<std::vector<std::string>>
	scan(std::string const& key1) =0;

	/** Create an entry `key1,key2`
	 * with the given value, and with
	 * some arbitrary starting generation.
	 * Returns the starting generation
	 * for the new mapping.
	 * Throws `KeyExists` if an entry
	 * already exists.
	 *
	 * Throws:
	 * - `Sidepool::Freed`
	 * - `KeyExists`
	 * - `IoError`
	 * - `OutOfSpace`
	 */
	virtual
	Sidepool::Io<uint32_t>
	create( std::string const& key1
	      , std::string const& key2
	      , std::string const& value
	      ) =0;

	/** Read an existing entry and
	 * return the value and generation.
	 *
	 * Throws:
	 * - `Sidepool::Freed`
	 * - `KeyNotFound`
	 * - `IoError`
	 */
	virtual
	Sidepool::Io<std::pair<std::uint32_t, std::string>>
	read( std::string const& key1
	    , std::string const& key2
	    ) =0;

	/** Like the above, but returns
	 * nullptr instead of throwing
	 * `KeyNotFound` if the `key1,key2`
	 * is not found.
	 *
	 * Not virtual, this just reuses
	 * `read`.
	 *
	 * Throws:
	 * - `Sidepool::Freed`
	 * - `IoError`
	 */
	Sidepool::Io<std::unique_ptr<std::pair<std::uint32_t, std::string>>>
	try_read( std::string const& key1
		, std::string const& key2
		);

	/** Change the given entry, if
	 * the given `generation` matches.
	 * If it matches, `generation`
	 * increases and is returned.
	 *
	 * Throws:
	 * - `Sidepool::Freed`
	 * - `KeyNotFound`
	 * - `WrongGeneration`
	 * - `IoError`
	 * - `OutOfSpace`
	 */
	virtual
	Sidepool::Io<std::uint32_t>
	update( std::string const& key1
	      , std::string const& key2
	      , std::uint32_t generation
	      , std::string const& value
	      ) =0;

	/** Read the given entry, call
	 * the given function, then
	 * write its returned value
	 * into that entry.
	 *
	 * Returns the value that
	 * was successfully written
	 * (i.e. the value the function
	 * returned).
	 *
	 * In case there is contention
	 * for updating the `key1,key2`,
	 * the given function may be
	 * called multiple times.
	 *
	 * Non-virtual, this is just
	 * built from `read` and
	 * `update`; atomicity is
	 * achieved via the `generation`
	 * counter.
	 *
	 * Throws:
	 * - `Sidepool::Freed`
	 * - `KeyNotFound`
	 * - `IoError`
	 * - `OutOfSpace`
	 * - anything the function throws.
	 */
	Sidepool::Io<std::string>
	rmw( Sidepool::Idler& idler
	   , std::string const& key1
	   , std::string const& key2
	   , std::function<Sidepool::Io<std::string>(std::string)> func
	   );

	/** Delete a `key1,key2` entry,
	 * if the generation matches.
	 *
	 * Throws:
	 * - `Sidepool::Freed`
	 * - `KeyNotFound`
	 * - `WrongGeneration`
	 * - `IoError`
	 * - `OutOfSpace`
	 */
	virtual
	Sidepool::Io<void>
	del( std::string const& key1
	   , std::string const& key2
	   , std::uint32_t generation
	   ) =0;

	/** Read the entry, then possibly
	 * create, update, or delete the
	 * entry.
	 *
	 * non-virtual; this is built from
	 * `create` `read` `update` `del`.
	 *
	 * Attempt to read the given entry.
	 * If not present, pass `nullptr`
	 * to the function, and if the
	 * function returns non-`nullptr`,
	 * attempt to create the entry.
	 * If present, pass the string to
	 * the function, and if it returns
	 * `nullptr`, attempt to delete
	 * the entry, or if it returns
	 * non-`nullptr`, attempt to
	 * update the entry.
	 *
	 * The given function may be
	 * called multiple times in
	 * case of contention for the
	 * given `key1,key2` entry.
	 *
	 * Throws:
	 * - `Sidepool::Freed`
	 * - `IoError`
	 * - `OutOfSpace`
	 */
	Sidepool::Io<std::unique_ptr<std::string>>
	crud( Sidepool::Idler& idler
	    , std::string const& key1
	    , std::string const& key2
	    , std::function<Sidepool::Io<std::unique_ptr<std::string>>(std::unique_ptr<std::string>)> func
	    );

	/** Construct an adaptor for the
	 * given C-based `libsidepool_saver`.
	 * This takes responsibility for
	 * the idler an clears the
	 * passed-in pointer if it is
	 * able to create the saver.
	 * If it throws, the passed-in
	 * pointer is not cleared.
	 */
	static
	std::unique_ptr<Saver>
	create(libsidepool_saver*& var);
};

}

#endif /* !defined(SIDEPOOL_SAVER_HPP_) */
