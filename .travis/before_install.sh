#!/bin/bash

die() {
	echo "$@"
	exit 1
}

case $OS in
	gentoo|Gentoo)
		# pull the image which includes dependencies
		docker pull hasufell/gentoo-pnmixer-test:latest \
			|| die "failed to pull docker image for $OS!"
		;;
	debian|Debian)
		# pull base image
		docker pull hasufell/debian-pnmixer-test:latest \
			|| die "failed to pull docker image for $OS!"
		;;
	ubuntu|Ubuntu)
		docker pull hasufell/ubuntu-pnmixer-test:latest \
			|| die "failed to pull docker image for $OS!"
		;;
	*)
		die "unsupported OS $OS!"
		;;
esac
