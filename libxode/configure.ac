AC_INIT([libxode],[1.2-jggtrans],[])
AC_CONFIG_SRCDIR([src/xmlnode.c])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(include/config.h)

AC_DISABLE_SHARED
AC_ENABLE_STATIC

AC_PROG_CC
AM_PROG_LIBTOOL
AC_PROG_INSTALL
AC_STDC_HEADERS
AC_C_BIGENDIAN

AM_PATH_EXPAT([1.95.1],[],[AC_MSG_ERROR([Expat is required, but not found])])
AC_CHECK_FUNC(snprintf,AC_DEFINE(HAVE_SNPRINTF,1,[Define if snprintf function is available]))

AC_CHECK_LIB(nsl, gethostent)
AC_CHECK_FUNC(connect,AC_DEFINE(HAVE_LIBSOCKET,1,[Define if libsocket library is used]),[
	  AC_CHECK_LIB(socket,connect,use_lib_socket=yes,AC_MSG_ERROR([
*************************************************************
*  You do not seem to have the socket call available as a   *
*  function or in the socket library.  This problem must be *
*  fixed before you can continue.                           *
*************************************************************]))])

AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(short)

AC_OUTPUT(Makefile \
    libxode-config \
	src/Makefile \
	include/Makefile)
