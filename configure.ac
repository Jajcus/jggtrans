# Process this file with autoconf to produce a configure script.
AC_INIT(jabber-gg-transport, 0.9,jajcus@pld.org.pl)
AM_INIT_AUTOMAKE(jabber-gg-transport, 0.9,jajcus@pld.org.pl)
AC_CONFIG_SRCDIR([src/ggtrans.h])
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_MAKE_SET
AM_PROG_LIBTOOL

AM_PATH_GLIB

# Checks for libraries.
AC_CHECK_LIB([efence], [main])
AC_CHECK_LIB([gg], [gg_login])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h errno.h fcntl.h netdb.h netinet/in.h stddef.h stdlib.h string.h strings.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_TYPE_SIGNAL
AC_FUNC_STAT

AC_CHECK_FUNCS([bzero gethostbyname gethostname isascii])
AC_CHECK_FUNCS([memmove memset modf select socket])
AC_CHECK_FUNCS([strcasecmp strchr strdup strncasecmp strstr])

AC_CONFIG_SUBDIRS([libxode])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
