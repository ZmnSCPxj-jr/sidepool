#if HAVE_CONFIG_H
# include"config.h"
#endif
#include"Sidepool/Crypto/memzero.hpp"

/*--------------------------------------------------*/
#if HAVE_GCC_ASM_CLEAR
namespace {
void libsidepool_protect(void* dat) {
	__asm__ __volatile__ ("" : : "r"(dat) : "memory");
}
}
/*--------------------------------------------------*/
#elif HAVE_ATTRIBUTE_WEAK
void libsidepool_protect_dummy(void* dat)
__attribute__((weak))
;
namespace {
void libsidepool_protect(void* dat) {
	if (libsidepool_protect_dummy) {
		libsidepool_protect_dummy(dat);
	}
}
}
/*--------------------------------------------------*/
#elif HAVE_PLAIN_ASM_CLEAR
namespace {
void libsidepool_protect(void* dat) {
	asm;
	(void) dat;
}
}
/*--------------------------------------------------*/
#else
namespace {
void libsidepool_protect_default(void* dat) {
	(void) dat;
}
}
/* This is deliberately made global so that the
per-file compiler will assume that the data is
accessible after the memzeroing, because an
external file might change this pointer to a
different function that DOES read the argument.
This does not work with link-time optimization,
which can see that this is never changed.

This is different for __attribute__((weak));
the symbol may resolve at dynamic linking time,
and even at link-time optimization the compiler
will assume that the dynamically-linked symbol
might actually read the memory after it is
zeroed.
*/
void (*libsidepool_protect)(void*) =
	&libsidepool_protect_default
;
#endif

/*--------------------------------------------------*/
#if HAVE_MEMSET_EXPLICIT
#include<string.h>
namespace {
void generic_memzero(void* dat, std::size_t len) {
	(void) memset_explicit(dat, 0, len);
	libsidepool_protect(dat);
}
}

/*--------------------------------------------------*/
#elif HAVE_MEMSET_S
#define __STDC_WANT_LIB_EXT1__ 1
#include<string.h>

namespace {
void generic_memzero(void* dat, std::size_t len) {
	(void) memset_s(dat, len, 0, len);
	libsidepool_protect(dat);
}
}

/*--------------------------------------------------*/
#elif HAVE_EXPLICIT_BZERO
/* Notice "strings" --- FreeBSD moved it some
versions ago.
*/
#include<strings.h>

namespace {
void generic_memzero(void* dat, std::size_t len) {
	explicit_bzero(dat, len);
	libsidepool_protect(dat);
}
}

/*--------------------------------------------------*/
#elif HAVE_WINDOWS_SECUREZEROMEMORY
#include<windows.h>
#include<wincrypt.h>

namespace {
void generic_memzero(void* dat, std::size_t len) {
	(void) SecureZeroMemory(dat, len);
	libsidepool_protect(dat);
}
}

/*--------------------------------------------------*/
#else
namespace {
void generic_memzero(void* dat, std::size_t len) {
	auto pdat = (char volatile*) dat;
	for (auto i = 0; i < len; ++i) {
		pdat[i] = 0;
	}
#if HAVE_GCC_ASM_CLEAR
	__asm__ __volatile__ ("" : : "r"(dat), "r"(pdat) : "memory");
#endif
	libsidepool_protect(dat);
	libsidepool_protect((void*) pdat);
}
}
#endif

namespace Sidepool::Crypto {
void memzero(void* dat, std::size_t len) {
	generic_memzero(dat, len);
}
}
