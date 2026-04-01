AC_INIT([mtc], [0.1.0], [page.cal@gmail.com])
AC_CONFIG_SRCDIR([mtc.c])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

# C compiler and archiver
AC_PROG_CC
AC_PROG_RANLIB
AM_PROG_AR

# pkg-config
PKG_PROG_PKG_CONFIG

# Required libraries
PKG_CHECK_MODULES([CURL], [libcurl])
PKG_CHECK_MODULES([JSONC], [json-c])
PKG_CHECK_MODULES([OPENSSL], [openssl])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
