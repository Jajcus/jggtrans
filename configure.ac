# Process this file with autoconf to produce a configure script.
AC_INIT([jabber-gg-transport], [2.0.8], [jajcus@pld.org.pl])
AC_CONFIG_SRCDIR([src/ggtrans.h])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER([config.h])

ALL_LINGUAS="pl"

# Checks for programs.
AC_PROG_CC
AC_C_BIGENDIAN
AC_PROG_MAKE_SET
AM_PROG_LIBTOOL
AM_GNU_GETTEXT([external])

CFLAGS="$CFLAGS -Wall"

AM_PATH_GLIB([1.2.0],[],AC_MSG_ERROR("You need glib >= 1.2.0 to compile this."))

AC_ARG_WITH([libgadu-snapshot],AC_HELP_STRING([--with-libgadu-snapshot],[Try to compile and link with some libgadu snapshot]))
if test "x$with_libgadu_snapshot" = "xyes" ; then
PKG_CHECK_MODULES([JK_LIBGADU], [libgadu >= 20030420])
else
PKG_CHECK_MODULES([JK_LIBGADU], [libgadu >= 1.0 libgadu < 20000000])
fi

AC_SUBST(JK_LIBGADU_LIBS)
AC_SUBST(JK_LIBGADU_CFLAGS)

AC_ARG_ENABLE([debug],AC_HELP_STRING([--disable-debug],[Disable debugging code]))

AC_ARG_WITH([efence],AC_HELP_STRING([--with-efence],[Link against ElectricFence library]))

if test "x$enable_debug" != "xno" ; then
	AC_DEFINE(DEBUG,1,[Define to enable debugging code])
fi

if test "x$with_efence" = "xyes" ; then
	AC_CHECK_LIB([efence], [malloc])
fi

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([stdlib.h string.h])
AC_CHECK_HEADERS([sys/socket.h netdb.h netinet/in.h])
AC_CHECK_HEADERS([sys/time.h unistd.h errno.h fcntl.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_TYPE_SIGNAL
AC_FUNC_STAT

AC_SEARCH_LIBS([gethostbyname],[resolve resolv],[],AC_MSG_ERROR("Some functions needed are missing"))
AC_SEARCH_LIBS([socket],[socket],[],AC_MSG_ERROR("Some functions needed are missing"))
AC_CHECK_FUNCS([isascii],[],AC_MSG_ERROR("Some functions needed are missing"))
AC_CHECK_FUNCS([select],[],AC_MSG_ERROR("Some functions needed are missing"))
AC_CHECK_FUNCS([memset strchr],[],AC_MSG_ERROR("Some functions needed are missing"))

AC_CONFIG_SUBDIRS([libxode])

MKINSTALLDIRS="${MKINSTALLDIRS} -m 755"
XGETTEXT="${XGETTEXT} --keyword=L_"

AC_CONFIG_FILES([Makefile src/Makefile m4/Makefile po/Makefile.in])
AC_OUTPUT
