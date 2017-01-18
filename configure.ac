dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.60])
AC_INIT([pnmixer], 0.7)

AM_INIT_AUTOMAKE([foreign])
AM_MAINTAINER_MODE
AC_CONFIG_HEADERS([src/config.h])

# ===================================================== #
#                  Basic compiler settings              #
# ===================================================== #
AC_PROG_CC
AC_PROG_INSTALL
IT_PROG_INTLTOOL([0.40])
# necessary for correct runtime behavior
LDFLAGS="$LDFLAGS -rdynamic"

# ======================================================= #
#                  I18n support                           #
# ======================================================= #
GETTEXT_PACKAGE=pnmixer
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext package.])
# Add more languages here
ALL_LINGUAS="de fr hr it nl ru sr uk vi zh_CN"
AM_GLIB_GNU_GETTEXT

# ======================================================= #
#                  Required packages                      #
# ======================================================= #
PKG_PROG_PKG_CONFIG

pkg_modules=""

# Check that we have -lm and add it to LIBS
AC_CHECK_LIB(m,ceil,,AC_MSG_FAILURE([libm not found. Check 'config.log' for more details.]))

# Make sure we have X11
echo -n "checking for x11... "
if ${PKG_CONFIG} --exists x11; then
        echo "yes"
        pkg_modules="$pkg_modules x11"
else
        echo "no"
        AC_MSG_ERROR([x11 not found])
fi

# Make sure we have Alsa
echo -n "checking for alsa... "
if ${PKG_CONFIG} --exists alsa; then
	echo "yes"
	pkg_modules="$pkg_modules alsa"
else
	echo "no"
	AC_MSG_ERROR([alsa not found])
fi

# ======================================================= #
#                  Gtk support                            #
# ======================================================= #
AC_ARG_WITH([gtk3],
            [AS_HELP_STRING([--with-gtk3], [Use Gtk3 as toolkit (def=yes)])],
            [with_gtk3="$withval"],
            [with_gtk3="yes"])

if test "$with_gtk3" = "yes"; then
	pkg_modules="$pkg_modules gtk+-3.0 >= 3.12.0"
	AC_DEFINE([WITH_GTK3], [], [Gtk3 mode])
else
	pkg_modules="$pkg_modules gtk+-2.0 >= 2.24.0"
fi
AM_CONDITIONAL([WITH_GTK3], [test "$with_gtk3" = "yes"])

# ======================================================= #
#                  Libnotify support                      #
# ======================================================= #
AC_ARG_WITH([libnotify],
            [AS_HELP_STRING([--with-libnotify], [Enable sending of notifications @<:@default=check@:>@])],
            [with_libnotify="$withval"],
            [with_libnotify="check"])

AS_IF([test "$with_libnotify" != no],
      [PKG_CHECK_EXISTS([libnotify], HAVE_LIBN=1, )]
     ,)

if test "$with_libnotify" = "yes" || test "$HAVE_LIBN" = "1"; then
	pkg_modules="$pkg_modules libnotify"
	AC_DEFINE([HAVE_LIBN], 1, [Defined if you have libnotify])
	HAVE_LIBN=1
fi

# ======================================================= #
#                  Check for modules                      #
# ======================================================= #
PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

AC_CONFIG_FILES([
Makefile
data/Makefile
data/desktop/Makefile
data/icons/Makefile
data/pixmaps/Makefile
data/ui/Makefile
man/Makefile
po/Makefile.in
src/Makefile
])

AC_OUTPUT

# ======================================================= #
#                  Display build configuration            #
# ======================================================= #
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

