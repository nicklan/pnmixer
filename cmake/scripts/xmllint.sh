#!/bin/sh

which xmllint >/dev/null ||
	{ echo "'xmllint' is required, please install it"; exit 1; }

for file in data/ui/*.glade; do
	mv "${file}" "${file}.bck"
	xmllint --format "${file}.bck" > "${file}"
	rm "${file}.bck"
done

