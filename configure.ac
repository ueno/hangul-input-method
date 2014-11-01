AC_PREREQ(2.63)
AC_INIT([hangul-input-method], [0.1],
        [bugs@example.com])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.12 dist-xz no-dist-gzip foreign tar-ustar no-define serial-tests -Wno-portability])
# revert automake using PACKAGE_TARNAME instead of PACKAGE_NAME
AC_SUBST([PACKAGE], [$PACKAGE_NAME])

AC_PROG_CC
AM_PROG_CC_C_O

PKG_CHECK_MODULES([DEPS], [gio-2.0 xkbcommon libhangul])

AC_CONFIG_FILES([
  Makefile
])
AC_OUTPUT


