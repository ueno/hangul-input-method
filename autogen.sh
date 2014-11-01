#!/bin/sh

test "$srcdir" || srcdir=.

autoreconf -f -i

if test x$NOCONFIGURE = x; then
    printbold Running $srcdir/configure $conf_flags "$@" ...
    $srcdir/configure $conf_flags "$@" \
        && echo Now type \`make\' to compile $PKG_NAME || exit 1
else
    echo Skipping configure process.
fi
