dnl Process this file with autoconf to produce a configure script.

AC_INIT([pnmixer], 0.6-rc2)
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_HEADER_STDC

IT_PROG_INTLTOOL([0.40])

AC_ARG_WITH([gtk3],
			  [AS_HELP_STRING([--with-gtk3],
							  [use gtk3 as toolkit (def=yes)])],
			  [with_gtk3="$withval"],
			  [with_gtk3=yes])

if test "$with_gtk3" = "yes" ; then
	pkg_modules="gtk+-3.0 >= 3.4.2"
	AC_DEFINE([WITH_GTK3], [], [Gtk3 mode])
else
	pkg_modules="gtk+-2.0 >= 2.16.0"
fi
AM_CONDITIONAL([WITH_GTK3], [test "$with_gtk3" = "yes"])

echo -n "checking for alsasound ... "
if pkg-config --exists  alsa ; then
        echo "yes"
pkg_modules="$pkg_modules alsa"
else
        echo "no"
        AC_MSG_ERROR(alsa not found)
fi

# Make sure we have X11, and also have it added to LIBS
AC_CHECK_LIB(X11,XOpenDisplay,,AC_MSG_FAILURE([libX11 not found. Check 'config.log' for more details.]))
# Check that we have -lm and add it to LIBS
AC_CHECK_LIB(m,ceil,,AC_MSG_FAILURE([libm not found. Check 'config.log' for more details.]))


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
	    CFLAGS="$CFLAGS -g -Wall -Werror -Wno-uninitialized  -Wno-deprecated-declarations -Wformat -Wformat-security -O0"
	fi
else
    AC_DEFINE([NDEBUG],[],[No-debug Mode])
	if test "$minimalflags" = "no" ; then
	    CFLAGS="$CFLAGS -O2"
	fi
fi

LDFLAGS="$LDFLAGS -rdynamic"

##########################################################################



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

# Needed by intltool
GETTEXT_PACKAGE=pnmixer
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

dnl Add the languages which your application supports here.
ALL_LINGUAS="de fr it ru"

AC_OUTPUT([
Makefile
data/Makefile
data/desktop/Makefile
data/icons/Makefile
data/pixmaps/Makefile
data/ui/Makefile
src/Makefile
po/Makefile.in
])

if test "$HAVE_LIBN" != "1"; then
	libnotify_msg="no"
else
	libnotify_msg="yes"
fi

if test "$with_gtk3" = "yes" ; then
	gtk_msg="3"
else
	gtk_msg="2"
fi

AS_ECHO([""])
AS_ECHO(["====================================="])
AS_ECHO(["CONFIGURATION:"])
AS_ECHO(["libnotify enabled..... $libnotify_msg"])
AS_ECHO(["gtk version........... $gtk_msg"])
AS_ECHO(["====================================="])
AS_ECHO([""])

