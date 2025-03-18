pkglib_LIBRARIES = libsidepool.a
include_HEADERS = libsidepool.h

AM_CXXFLAGS = -Wall -Werror -fsanitize=address
if USE_FSANITIZE_ADDRESS
AM_CXXFLAGS += -fsanitize=address
endif
if USE_FSANITIZE_LEAK
AM_CXXFLAGS += -fsanitize=leak
endif

libsidepool_a_SOURCES = \
	Sidepool/Freed.hpp \
	Sidepool/Io.hpp \
	Sidepool/Idler.cpp \
	Sidepool/Idler.hpp \
	Sidepool/String.cpp \
	Sidepool/String.hpp \
	Sidepool/stringify.hpp \
	libsidepool.h

EXTRA_DIST = CONTRIBUTING.md

ACLOCAL_AMFLAGS = -I m4

# Use valgrind on tests if available
if USE_VALGRIND
LOG_COMPILER = valgrind
AM_LOG_FLAGS = --leak-check=full --error-exitcode=1
AM_CXXFLAGS += -DUSE_VALGRIND
else
LOG_COMPILER = env
AM_LOG_FLAGS =
if USE_FSANITIZE_LEAK
AM_LOG_FLAGS += ASAN_OPTIONS="detect_leaks=1"
endif
endif

# These will be loaded by the test suite.
LDADD = libsidepool.a

# Tests are written in C++.
AM_DEFAULT_SOURCE_EXT = .cpp
TESTS = \
	tests/idler/test_methods \
	tests/string/test_fmt \
	tests/stringify/test_simple
check_PROGRAMS = $(TESTS)

