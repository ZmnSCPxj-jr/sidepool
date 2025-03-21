pkglib_LIBRARIES = libsidepool.a
include_HEADERS = libsidepool.h

AM_CXXFLAGS = -Wall -Werror
if USE_FSANITIZE_ADDRESS
AM_CXXFLAGS += -fsanitize=address
endif
if USE_FSANITIZE_LEAK
AM_CXXFLAGS += -fsanitize=leak
endif

libsidepool_a_SOURCES = \
	Sidepool/Crypto/CSPRNG.cpp \
	Sidepool/Crypto/CSPRNG.hpp \
	Sidepool/Crypto/SHA256.cpp \
	Sidepool/Crypto/SHA256.hpp \
	Sidepool/Crypto/chacha20_ietf.cpp \
	Sidepool/Crypto/chacha20_ietf.hpp \
	Sidepool/Freed.hpp \
	Sidepool/Io.hpp \
	Sidepool/Idler.cpp \
	Sidepool/Idler.hpp \
	Sidepool/Logger.cpp \
	Sidepool/Logger.hpp \
	Sidepool/Main.cpp \
	Sidepool/Main.hpp \
	Sidepool/Math.cpp \
	Sidepool/Math.hpp \
	Sidepool/Mod/all.cpp \
	Sidepool/Mod/all.hpp \
	Sidepool/Msg/ProvideSaver.hpp \
	Sidepool/Msg/Start.hpp \
	Sidepool/Randomizer.cpp \
	Sidepool/Randomizer.hpp \
	Sidepool/S/Bus.cpp \
	Sidepool/S/Bus.hpp \
	Sidepool/S/Detail/Lease.hpp \
	Sidepool/S/Detail/Registry.cpp \
	Sidepool/S/Detail/Registry.hpp \
	Sidepool/S/Detail/TypedLease.hpp \
	Sidepool/S/Detail/TypedRegistry.hpp \
	Sidepool/S/Lessee.cpp \
	Sidepool/S/Lessee.hpp \
	Sidepool/Saver.cpp \
	Sidepool/Saver.hpp \
	Sidepool/String.cpp \
	Sidepool/String.hpp \
	Sidepool/Util.hpp \
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

# Testing/ includes some header files used for
# testing only.
# Header files only!
TESTING_HEADER_FILES = \
	Testing/Idler.hpp

noinst_HEADERS = $(TESTING_HEADER_FILES)

# These will be loaded by the test suite.
LDADD = libsidepool.a

# Tests are written in C++.
AM_DEFAULT_SOURCE_EXT = .cpp
TESTS = \
	tests/crypto/test_sha256 \
	tests/idler/test_methods \
	tests/s/test_kill_lessee \
	tests/s/test_simple_bus \
	tests/string/test_fmt \
	tests/string/test_hex \
	tests/stringify/test_simple
check_PROGRAMS = $(TESTS)

