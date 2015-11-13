#!/bin/bash

die() {
	echo "$@"
	exit 1
}

case $OS in
	gentoo|Gentoo)
		docker run -ti \
			-v "`pwd`":/pnmixer \
			hasufell/gentoo-pnmixer-test:latest \
			sh -c "cd /pnmixer && CC=$CC CFLAGS=\"$CFLAGS\" ./autogen.sh $BUILD_FLAGS && make" \
			|| die "failed to build image on $OS"
		;;
	debian|Debian)
		docker run -ti \
			-v "`pwd`":/pnmixer \
			hasufell/debian-pnmixer-test:latest \
			sh -c "cd /pnmixer && CC=$CC CFLAGS=\"$CFLAGS\" ./autogen.sh $BUILD_FLAGS && make" \
			|| die "failed to build image on $OS"
		;;
	ubuntu|Ubuntu)
		docker run -ti \
			-v "`pwd`":/pnmixer \
			hasufell/ubuntu-pnmixer-test:latest \
			sh -c "cd /pnmixer && CC=$CC CFLAGS=\"$CFLAGS\" ./autogen.sh $BUILD_FLAGS && make" \
			|| die "failed to build image on $OS"
		;;
	*)
		die "unsupported OS: $OS!"
		;;
esac

