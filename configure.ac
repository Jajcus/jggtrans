# Process this file with autoconf to produce a configure script.
AC_INIT(jabber-gg-transport, 0.9,jajcus@pld.org.pl)
AM_INIT_AUTOMAKE(jabber-gg-transport, 0.9,jajcus@pld.org.pl)
AC_CONFIG_SRCDIR([src/ggtrans.h])
AM_CONFIG_HEADER(config.h)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_C_BIGENDIAN
AC_PROG_MAKE_SET
AM_PROG_LIBTOOL

AM_PATH_GLIB(1.2.0,[],AC_MSG_ERROR("You need glib >= 1.2.0 to compile this."))

# Checks for libraries.
AC_CHECK_LIB([gg], [gg_login],[],AC_MSG_ERROR("You need libgg (from ekg package) to compile this."))

AC_ARG_ENABLE(debug,
		AC_HELP_STRING([--enable-debug],[Enable debugging code]),
		[DEBUG=yes],[DEBUG=no])

AH_TEMPLATE([DEBUG],[Define to enable debugging code])
if test "x$DEBUG" = "xyes" ; then
AC_DEFINE(DEBUG)
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

AC_CHECK_FUNCS([gethostbyname isascii],[],AC_MSG_ERROR("Some functions needed are missing"))
AC_CHECK_FUNCS([select socket],[],AC_MSG_ERROR("Some functions needed are missing"))
AC_CHECK_FUNCS([memset strchr],[],AC_MSG_ERROR("Some functions needed are missing"))

AC_CONFIG_SUBDIRS([libxode])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
