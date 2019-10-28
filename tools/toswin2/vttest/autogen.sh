#!/bin/sh

aclocal -I m4 && autoconf && autoheader
rm -rf autom4te.cache
