dnl Process this file with autoconf to produce a configure script.

AC_INIT([pnmixer], 0.6-dev)
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

OLD_CFLAGS=$CFLAGS
AC_ISC_POSIX
AC_PROG_CC
AC_HEADER_STDC
CFLAGS=$OLD_CXXFLAGS

pkg_modules="gtk+-3.0 >= 3.6.0"

echo -n "checking for alsasound ... "
if pkg-config --exists  alsa ; then
        echo "yes"
pkg_modules="$pkg_modules alsa"
else
        echo "no"
        AC_MSG_ERROR(alsa not found)
fi

##########################################################################
# debug compilation support
##########################################################################

AC_MSG_CHECKING([whether to build with debug information])
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
        [enable debug data generation (def=no)])],
    [debugit="$enableval"],
    [debugit=no])
AC_MSG_RESULT([$debugit])

AC_ARG_ENABLE([minimal-flags],
			  [AS_HELP_STRING([--enable-minimal-flags],
							  [respect system CFLAGS (def=no)])],
			  [minimalflags="$enableval"],
			  [minimalflags=no])

if test x"$debugit" = x"yes"; then
    AC_DEFINE([DEBUG],[],[Debug Mode])
	if test "$minimalflags" = "no" ; then
	    CFLAGS="$CFLAGS -g -Wall -Werror -Wno-uninitialized -Wformat -Wformat-security -O0 -export-dynamic"
	fi
else
    AC_DEFINE([NDEBUG],[],[No-debug Mode])
	if test "$minimalflags" = "no" ; then
	    CFLAGS="$CFLAGS -O2 -export-dynamic"
	fi
fi

##########################################################################

# Make sure we have X11, and also have it added to LIBS
AC_CHECK_LIB(X11,XOpenDisplay,,AC_MSG_FAILURE([libX11 not found. Check 'config.log' for more details.]))
# Check that we have -lm and add it to LIBS
AC_CHECK_LIB(m,ceil,,AC_MSG_FAILURE([libm not found. Check 'config.log' for more details.]))


#######################
# Check for libnotify #
#######################
AC_ARG_WITH(libnotify,
   [AS_HELP_STRING([--with-libnotify],
      [Enable sending of notifications @<:@default=check@:>@])],
   [with_libnotify=$withval],
   [with_libnotify="check"])

AS_IF([test "x$with_libnotify" != xno],
  [PKG_CHECK_EXISTS([libnotify], HAVE_LIBN=1, )]
,)

if test "$with_libnotify" = "yes" || test "$HAVE_LIBN" = "1"; then
   PKG_CHECK_MODULES(LIBNOTIFY, [libnotify], 
                     [],
                     AS_IF([test "x$with_libnotify" != xcheck],[AC_MSG_FAILURE(["--with-libnotify was given but test for libnotify failed"])]))
   HAVE_LIBN=1
   AC_SUBST(LIBNOTIFY_CFLAGS)
   AC_SUBST(LIBNOTIFY_LIBS)
   AC_DEFINE([HAVE_LIBN],1,[Defined if you have libnotify])
fi

PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

GETTEXT_PACKAGE=pnmixer
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

dnl Add the languages which your application supports here.
ALL_LINGUAS="de fr it"
AM_GLIB_GNU_GETTEXT

AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
])

if test "$HAVE_LIBN" != "1"; then
  AS_ECHO(["-----------------------------------------------NOTICE-------------------------------------------------"])
  AS_ECHO(["| libnotify not found on your system, or explicitly disabled.  Notification support will be disabled |"])
  AS_ECHO(["------------------------------------------------------------------------------------------------------"])
fi


