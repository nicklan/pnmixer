#!/bin/bash

die() {
	echo "$@"
	exit 1
}

case $OS in
	gentoo|Gentoo)
		;;
	debian|Debian)
		;;
	*)
		die "unsupported OS $OS!"
		;;
esac
