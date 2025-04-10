#if HAVE_CONFIG_H
# include"config.h"
#endif
#undef NDEBUG
#include"Sidepool/SerDe/Archive.hpp"
#include"Sidepool/SerDe/serialize.hpp"
#include<cassert>

namespace {

template<typename T>
void test(T& obj) {
	/* Create the writer.  */
	auto writer_vec = Sidepool::SerDe::Archive::make_writing_archive();
	auto writer = std::move(writer_vec.first);
	auto vec = std::move(writer_vec.second);
	/* Write it out, which should fill in the
	vec.
	*/
	Sidepool::SerDe::serialize(*writer, obj);

	/* Create the reader.  */
	auto reader = Sidepool::SerDe::Archive::make_reading_archive(std::move(*vec));
	/* Read in a new instance.  */
	auto nobj = T();
	Sidepool::SerDe::serialize(*reader, nobj);

	/* Check they are equal.  */
	assert(obj == nobj);
}

struct Example {
	char c;
	std::uint8_t u8;
	std::uint16_t u16;
	std::uint32_t u32;
	std::uint64_t u64;
	std::int8_t i8;
	std::int16_t i16;
	std::int32_t i32;
	std::int64_t i64;
	std::string s;
	std::unique_ptr<std::string> us;

	Example() {
		c = 0;
		u8 = 0;
		u16 = 0;
		u32 = 0;
		u64 = 0;
		i8 = 0;
		i16 = 0;
		i32 = 0;
		i64 = 0;
		s = "";
		us = nullptr;
	}

	bool operator==(Example const& o) const {
		return ( (c == o.c)
		      && (u8 == o.u8)
		      && (u16 == o.u16)
		      && (u32 == o.u32)
		      && (u64 == o.u64)
		      && (i8 == o.i8)
		      && (i16 == o.i16)
		      && (i32 == o.i32)
		      && (i64 == o.i64)
		      && (s == o.s)
		/* comparison of std::unique_ptr
		is pointer equality, but
		deserialization will create a new
		object.
		*/
		      && ((us && o.us) ? (*us == *o.us) : (!us && !o.us))
		       );
	}
};

}

namespace Sidepool::SerDe {

template<>
struct Trait<Example> {
	static
	void serialize( Archive& a
		      , Example& e
		      ) {
		Sidepool::SerDe::serialize(a, e.c);
		Sidepool::SerDe::serialize(a, e.u8);
		Sidepool::SerDe::serialize(a, e.u16);
		Sidepool::SerDe::serialize(a, e.u32);
		Sidepool::SerDe::serialize(a, e.u64);
		Sidepool::SerDe::serialize(a, e.i8);
		Sidepool::SerDe::serialize(a, e.i16);
		Sidepool::SerDe::serialize(a, e.i32);
		Sidepool::SerDe::serialize(a, e.i64);
		Sidepool::SerDe::serialize(a, e.s);
	}
};

}

int main () {

	{
		auto s = std::string("I am a cute cute kitty, a cute cute kitty, the cutest kitty kitty");
		test(s);
	}

	{
		auto i = std::uint64_t(0xF01EB8A39E783F3D);
		test(i);
	}

	{
		auto v = std::vector<std::string>();
		v.emplace_back("Orange cats are stupid");
		v.emplace_back("MEOW");
		test(v);
	}

	{
		auto e = Example();
		e.c = 'a';
		e.u8 = 42;
		e.u16 = 256;
		e.u32 = 65537;
		e.u64 = 0xFFFFFFFFFFFFFFFF;
		e.i8 = -42;
		e.i16 = -32767;
		e.i32 = -1048576;
		e.i64 = -1;
		e.s = "Cute cats are cute (unless orange)";
		test(e);
	}

	return 0;
}
