dnl required version of autoconf
AC_PREREQ([2.53])

strictness="foreign"

AC_INIT([gsh],[0.0.0])

AC_CONFIG_SRCDIR([.])
AC_CONFIG_HEADERS([config.h])

dnl required version of automake
AM_INIT_AUTOMAKE([1.10 foreign])
AM_SILENT_RULES([yes])
dnl enable mainainer mode by default
AM_MAINTAINER_MODE([enable])

dnl check for tools (compiler etc.)
AC_PROG_CC

dnl required version of libtool
LT_PREREQ([2.2.6])
LT_INIT

dnl give error and exit if we don't have pkgconfig
AC_CHECK_PROG(HAVE_PKGCONFIG, pkg-config, [ ], [
  AC_MSG_ERROR([You need to have pkg-config installed!])
])

PKG_CHECK_MODULES(GOBJ, [gobject-2.0], [
  AC_SUBST(GOBJ_CFLAGS)
  AC_SUBST(GOBJ_LIBS)
], [
  AC_MSG_ERROR([GObject library not found])
])

PKG_CHECK_MODULES(GMODULE, [gmodule-2.0], [
  AC_SUBST(GMODULE_CFLAGS)
  AC_SUBST(GMODULE_LIBS)
], [
  AC_MSG_ERROR([GModule library not found])
])

COMMON_CFLAGS="-I\$(top_srcdir)"
BASECLASS_CFLAGS="$COMMON_CFLAGS $GOBJ_CFLAGS"
BASECLASS_LIBS="$GOBJ_LIBS"
PLUGIN_CFLAGS="$COMMON_CFLAGS -fpic"
PLUGIN_LDFLAGS="-shared -module -avoid-version -no-undefined"

AC_SUBST(BASECLASS_CFLAGS)
AC_SUBST(BASECLASS_LIBS)

AC_SUBST(PLUGIN_LDFLAGS)
AC_SUBST(PLUGIN_CFLAGS)
AC_SUBST(PLUGIN_LDFLAGS)

dnl check if compiler understands -Wall (if yes, add -Wall)
AC_MSG_CHECKING([to see if compiler understands -Wall])
save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Wall -Werror"

AC_CONFIG_FILES([
Makefile
base/Makefile
])
AC_OUTPUT
