#!/bin/sh
# Run this to generate all the initial makefiles, etc.

echo "Running $0 ..."

autoreconf --force --install --warnings=all --verbose || exit $?

conf_flags="--enable-maintainer-mode"

if test x$NOCONFIGURE = x; then
  echo Running ./configure $conf_flags "$@" ...
  ./configure $conf_flags "$@" \
  && echo Now type \`make\' to compile. || exit 1
else
  echo Skipping configure process.
fi
