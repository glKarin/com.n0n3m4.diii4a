#!/bin/sh
# Generates Makefiles and more.

srcdir=`dirname $0`
(autoconf --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "You must have autoconf installed to compile $PROJECT."
    echo "Download the appropriate package for your distribution,"
    echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
    exit 1
}
set -e -x
autoreconf --warnings=all --install --verbose --force
