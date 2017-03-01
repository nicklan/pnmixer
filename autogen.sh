#!/bin/sh
# Run this to generate all the initial makefiles, etc.

echo "Running $0 ..."

# Install the pre-commit hook
if [ -f .githooks/pre-commit ] && [ ! -f .git/hooks/pre-commit ]; then
        # This part is allowed to fail
        cp -p .githooks/pre-commit .git/hooks/pre-commit && \
        chmod +x .git/hooks/pre-commit && \
        echo "Activated pre-commit hook." || :
fi

# Autoreconf
autoreconf --force --install --warnings=all --verbose || exit $?

# Configure
conf_flags="--enable-maintainer-mode"

if test x$NOCONFIGURE = x; then
  echo Running ./configure $conf_flags "$@" ...
  ./configure $conf_flags "$@" \
  && echo Now type \`make\' to compile. || exit 1
else
  echo Skipping configure process.
fi
