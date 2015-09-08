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
	ubuntu|Ubuntu)
		;;
	*)
		die "unsupported OS $OS!"
		;;
esac
