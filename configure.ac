# Process this file with autoconf to produce a configure script.
AC_INIT([jabber-gg-transport], [1.2.4-test2], [jajcus@pld.org.pl])
AC_CONFIG_SRCDIR([src/ggtrans.h])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_C_BIGENDIAN
AC_PROG_MAKE_SET
AM_PROG_LIBTOOL

AM_PATH_GLIB([1.2.0],[],AC_MSG_ERROR("You need glib >= 1.2.0 to compile this."))

# Checks for libraries.
AC_ARG_WITH(pthread,
  [  --with-pthread          Assume libgadu is compiled with pthread],[],[
	AC_MSG_CHECKING([If libgadu is compiled with pthread support])
	jk_gg_pthread=no
	if test -x /usr/bin/ldd ; then
		for l in /usr/lib/libgadu.so /usr/local/lib/libgadu.so ; do
			if test -f $l ; then
				if /usr/bin/ldd $l | grep -q "pthread" ; then	
					jk_gg_pthread=yes
				fi
				break;
			fi
		done
	fi
	if test "x$jk_gg_pthread" = "xyes" ; then
		AC_MSG_RESULT([yes])
		with_pthread=yes
	else	
		with_pthread=no
		AC_MSG_RESULT([no])
	fi
])

if test "x$with_pthread" = "xyes"; then
  AC_CHECK_HEADER(pthread.h, [
    AC_CHECK_LIB(pthread, pthread_create, [
      AC_DEFINE(HAVE_PTHREAD, 1, [Define if you want to use pthread in resolver])
      LIBS="$LIBS -lpthread"
      jk_use_pthread="yes"
    ],[
      jk_use_pthread="no"
    ])
  ])
else
	jk_use_pthread="no"
fi

if test "x$jk_use_pthread" = "xyes"; then
	AC_MSG_WARN([Jggtrans _will_ be compiled with pthread])
else
	AC_MSG_WARN([jggtrans _will_not_ be compiled with pthread])
fi
AC_MSG_WARN([  if this doesn't match libgadu configuration jggtrans may crash])

AC_CHECK_LIB([gadu], [gg_change_info],[],AC_MSG_ERROR("You need libgadu (from ekg>=20020807 package) to compile this."))

AC_ARG_ENABLE([debug],AC_HELP_STRING([--disable-debug],[Disable debugging code]))

AC_ARG_ENABLE([remote-userlist],AC_HELP_STRING([--enable-remote-userlist],[Enable support for contact lists on server]))

AC_ARG_WITH([efence],AC_HELP_STRING([--with-efence],[Link against ElectricFence library]))

if test "x$enable_debug" != "xno" ; then
	AC_DEFINE(DEBUG,1,[Define to enable debugging code])
fi

if test "x$enable_remote_userlist" = "xyes" ; then
	AC_DEFINE(REMOTE_USERLIST,1,[Define to enable remote userlist support])
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
AC_NEED_STDINT_H

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
AC_CHECK_LIB([gadu], [gg_event_free],[],AC_MSG_ERROR("You need libgadu>=20020807."))

AC_CONFIG_SUBDIRS([libxode])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
