#if HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/SerDe/Archive.hpp"
#include"Sidepool/SerDe/Eof.hpp"

namespace {

class ReadingArchive : public Sidepool::SerDe::Archive {
private:
	typedef std::vector<std::uint8_t>
		vector;
	vector data;
	vector::const_iterator it;
	vector::const_iterator end;

public:
	ReadingArchive() =delete;
	ReadingArchive(ReadingArchive&&) =delete;
	ReadingArchive(ReadingArchive const&) =delete;
	explicit
	ReadingArchive( vector data_
		      ) : data(std::move(data_))
			{
		it = data.cbegin();
		end = data.cend();
	}

	~ReadingArchive() override { }

	void ser_byte(std::uint8_t& v) override {
		if (it == end) {
			throw Sidepool::SerDe::Eof();
		}
		v = *it;
		++it;
	}
	bool is_reading() const override {
		return true;
	}
};

class WritingArchive : public Sidepool::SerDe::Archive {
private:
	typedef std::vector<std::uint8_t>
		vector;
	std::shared_ptr<vector> building;

public:
	WritingArchive() =delete;
	WritingArchive(WritingArchive&&) =delete;
	WritingArchive(WritingArchive const&) =delete;
	explicit
	WritingArchive( std::shared_ptr< vector
				       > building_
		      ) : building(std::move(building_))
			  { }

	~WritingArchive() override { }

	void ser_byte(std::uint8_t& v) override {
		building->push_back(v);
	}
	bool is_reading() const override {
		return false;
	}
};

}

namespace Sidepool::SerDe {

std::unique_ptr<Archive>
Archive::make_reading_archive(
		std::vector<std::uint8_t> data
) {
	return std::make_unique<ReadingArchive>(
		std::move(data)
	);
}

std::pair< std::unique_ptr<Archive>
	 , std::shared_ptr<std::vector<std::uint8_t>>
	 >
Archive::make_writing_archive() {
	auto building = std::make_shared<std::vector<std::uint8_t>>();
	auto rv = std::make_unique<WritingArchive>(
		building
	);
	return std::make_pair( std::move(rv)
			     , std::move(building)
			     );
}

}
