#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([libsidepool], [0.1A], [ZmnSCPxj@protonmail.com])
AC_CONFIG_SRCDIR([libsidepool.h])
AC_CONFIG_AUX_DIR([auxdir])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror subdir-objects 1.13])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_RANLIB
AM_PROG_AR
AC_PROG_CXX
AC_PROG_CC

AX_CXX_COMPILE_STDCXX([17], [noext])

AX_VALGRIND_CHECK()
AM_CONDITIONAL([USE_VALGRIND], [test x"$enable_valgrind" = xyes])

AC_LANG([C++])

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdint.h],[#then
	:
],[#else
	AC_MSG_ERROR([stdint.h not found, your system is too old])
])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

AC_MSG_CHECKING([if compiler accepts -fsanitize=address])
saved_CXXFLAGS=${CXXFLAGS}
CXXFLAGS="${CXXFLAGS} -fsanitize=address"
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
int main() { return 0; }
]])], [#then
    use_fsanitize_address=yes
], [#else
    use_fsanitize_address=no
])
CXXFLAGS="${saved_CXXFLAGS}"
AC_MSG_RESULT([${use_fsanitize_address}])
AM_CONDITIONAL([USE_FSANITIZE_ADDRESS], [test x"$use_fsanitize_address" = xyes])

AC_MSG_CHECKING([if compiler accepts -fsanitize=leak])
saved_CXXFLAGS=${CXXFLAGS}
CXXFLAGS="${CXXFLAGS} -fsanitize=leak"
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
int main() { return 0; }
]])], [#then
    use_fsanitize_leak=yes
], [#else
    use_fsanitize_leak=no
])
CXXFLAGS="${saved_CXXFLAGS}"
AC_MSG_RESULT([${use_fsanitize_leak}])
AM_CONDITIONAL([USE_FSANITIZE_LEAK], [test x"$use_fsanitize_leak" = xyes])

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
