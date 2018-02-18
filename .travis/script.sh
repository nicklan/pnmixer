#!/bin/bash

set -e

edo() {
    echo "$@" 1>&2
    "$@"
}

edo apt-get update
edo apt-get install -y clang-5.0 cmake doxygen graphviz gettext libasound2-dev libgtk-3-dev libgtk2.0-dev libnotify-dev ninja-build

edo /usr/share/clang/scan-build-5.0/libexec/ccc-analyzer --help
edo /usr/lib/llvm-5.0/libexec/ccc-analyzer --help

edo cd /pnmixer
edo mkdir build
edo cd build

# for clang, do static analysis
if [[ ${CC} == "clang" ]] ; then
	clang_ver="5.0"
	build_wrapper="scan-build-${clang_ver} --status-bugs --use-analyzer=/usr/bin/clang-${clang_ver}"
	export CC="/usr/lib/llvm-${clang_ver}/libexec/ccc-analyzer"
else
	build_wrapper=""
fi

edo cmake \
	-DWITH_GTK3=${WITH_GTK3} \
	-DWITH_LIBNOTIFY=${WITH_LIBNOTIFY} \
	-DENABLE_NLS=${ENABLE_NLS} \
	-DBUILD_DOCUMENTATION=ON \
	-G "${CMAKE_GENERATOR}" \
	-DCMAKE_C_COMPILER="${CC}" \
	-DCMAKE_C_FLAGS="-Wall -Wextra -Wno-deprecated-declarations -Werror" \
	..


if [[ "${CMAKE_GENERATOR}" == "Ninja" ]] ; then
	build_command="ninja -v"
	export DESTDIR="$(pwd)/install"
	install_command="ninja install"
else
	build_command="make VERBOSE=1"
	install_command="make DESTDIR=$(pwd)/install install"
fi

edo ${build_wrapper} ${build_command}
edo ${install_command}

