#!/bin/sh

which astyle >/dev/null ||
	{ echo "'astyle' is required, please install it"; exit 1; }

sed -i 's/[ \t]*$//' src/*.[ch]

astyle \
	--style=linux \
	--indent=tab=8 \
	--align-pointer=name \
	--suffix=none \
	src/*.[ch]

