#!/bin/sh

if [ -f .githooks/pre-commit ] &&
		[ -d .git ] &&
		[ ! -f .git/hooks/pre-commit ]; then
	cp -p .githooks/pre-commit .git/hooks/pre-commit &&
	chmod +x .git/hooks/pre-commit &&
	echo '-- Activated pre-commit hook.' || :
fi

