#pragma once
#ifndef SIDEPOOL_SERDE_ARCHIVE_HPP_
#define SIDEPOOL_SERDE_ARCHIVE_HPP_

#include<cstdint>
#include<memory>
#include<utility>
#include<vector>

namespace Sidepool::SerDe {

/** class Sidepool::SerDe::Archive
 *
 * @desc An abstract interface to an object
 * that either serializes to or deserializes
 * to some other thing.
 */
class Archive {
public:
	virtual ~Archive() { }

	/** If the archive is reading, this
	will overwrite the given `std::uint8_t`
	with the equivalent value in the archive.
	If the archive is writing, this will
	write the current value of the given
	`std::uint8_t` to the archive.
	*/
	virtual void ser_byte(std::uint8_t& v) =0;

	/** Return true if the archive is a
	reading archive (i.e. it will overwrite
	the value in `ser_byte`).
	*/
	virtual bool is_reading() const =0;
	/** Return true if the archive is a
	writing archive (i.e. it will read the
	value in `ser_byte` and persist it
	somewhere)
	*/
	bool is_writing() const {
		return !is_reading();
	}

	/** Create a reading archive that
	reads bytes from the given vector.
	*/
	static
	std::unique_ptr<Archive>
	make_reading_archive(std::vector<std::uint8_t>);

	/** Create a writing archive that
	writes bytes into a vector it is
	appending to, and which will be
	shared with the caller.
	*/
	static
	std::pair< std::unique_ptr<Archive>
		 , std::shared_ptr<std::vector<std::uint8_t>>
		 >
	make_writing_archive();
};

}

#endif /* !defined(SIDEPOOL_SERDE_ARCHIVE_HPP_) */
