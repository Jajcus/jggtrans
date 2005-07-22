dnl $Id: configure.in,v 1.154 2004/06/28 10:13:09 deletek Exp $

AC_INIT([libgadu], [20050128.jggtrans.1], [])
AC_CONFIG_SRCDIR([lib/libgadu.h])
AM_INIT_AUTOMAKE

AC_PREREQ(2.50)
AM_CONFIG_HEADER([config.h])
AC_DISABLE_SHARED
AC_PROG_LIBTOOL

AC_SUBST(CFLAGS_LIBGADU)
AC_SUBST(LIBS_LIBGADU)
AC_SUBST(OBJS)
AC_SUBST(IOCTLD_OBJS)
AC_SUBST(PC_REQUIRES)

AC_SUBST(strip_ekg, "")
AC_SUBST(strip_ioctld, "")
AC_SUBST(strip_libgadu_so, "")

AC_SUBST(ioctld, "")
AC_SUBST(install_ioctld, "")

AC_SUBST(shared, "")
AC_SUBST(install_shared, "")

AC_SUBST(lgadu, "")
AC_SUBST(libgadu_a, "")

AC_SUBST(static, "static")
AC_SUBST(install_static, "install-static")

AC_SUBST(ekg, "")
AC_SUBST(install_ekg, "")
AC_SUBST(clean_ekg, "")
AC_SUBST(distclean_ekg, "")
AC_SUBST(uninstall_ekg, "")
AC_SUBST(dep_ekg, "")

AC_SUBST(make_ekgwap, "")
AC_SUBST(install_ekgwap, "")

dnl
dnl  Zaczynamy testy...
dnl

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_C_CONST

AC_PATH_PROG(GMAKE, gmake, no)

if test "x$GMAKE" = "xno"; then
	AC_PATH_PROG(MAKE, make, no)

	if test "x$MAKE" = "xno"; then
		AC_MSG_ERROR([make ])
	fi
else
	MAKE="$GMAKE"
fi

AC_CHECK_TOOL(AR, ar, no)

if test "x$AR" = "xno"; then
	AC_MSG_ERROR([ar ])
fi

AC_CHECK_TOOL(STRIP, strip, :)

dnl
dnl  Tworzymy osobny plik lib/libgadu-config.h niezale¿ny od autoconfa
dnl  z w³asn± przestrzeni± nazw (__GG_LIBGADU), ¿eby móc go spokojnie
dnl  w³±czyæ w <libgadu.h>
dnl

mkdir -p lib

AC_DEFUN([AC_LIBGADU_DEFINE],
[
	sed 's/#undef __GG_LIBGADU_$1/#define __GG_LIBGADU_$1/' < lib/libgadu-config.h > lib/libgadu-config.h-
	mv -f lib/libgadu-config.h- lib/libgadu-config.h
])

AC_DEFUN([AC_LIBGADU_UNDEF],
[
	sed 's/#undef __GG_LIBGADU_$1.*/\/* & *\//' < lib/libgadu-config.h > lib/libgadu-config.h-
	mv -f lib/libgadu-config.h- lib/libgadu-config.h
])

echo "/* Generated automatically by configure. Do not edit. */" > lib/libgadu-config.h
cat ${srcdir}/lib/libgadu-config.h.in >> lib/libgadu-config.h

dnl
dnl  Najpierw sprawdzamy te opcje, które s± wspólne dla libgadu i ekg.
dnl

AC_C_BIGENDIAN

if test "x$ac_cv_c_bigendian" = "xyes"; then
	AC_LIBGADU_DEFINE([BIGENDIAN])
fi

AC_NEED_STDINT_H

if test "x$STDINT_H" = "x"; then
	cp stdint.h lib/libgadu-stdint.h
	STDINT_H=libgadu-stdint.h 
fi

sed "s/#include <stdint.h>/#include <$STDINT_H>/" < lib/libgadu-config.h > lib/libgadu-config.h-
mv -f lib/libgadu-config.h- lib/libgadu-config.h

dnl SunOS
AC_CHECK_LIB(nsl, t_accept, LIBS="$LIBS -lnsl")
AC_CHECK_LIB(socket, socket, LIBS="$LIBS -lsocket")

dnl BeOS

AC_ARG_WITH(bind,
  [  --without-bind          Disable linking with libbind when found])

if test "x$with_bind" != "xno"; then
	AC_CHECK_LIB(bind, __inet_addr, LIBS="$LIBS -lbind")
fi

dnl threadsafe
AC_CHECK_FUNCS([gethostbyname_r], [AC_LIBGADU_DEFINE([HAVE_GETHOSTBYNAME_R])])

dnl inet_pton
AC_CHECK_FUNCS(inet_pton)

AC_MSG_CHECKING([for va_copy])
AC_TRY_LINK([#include <stdarg.h>], [va_list a, b; va_copy(a, b);], 
[
	AC_MSG_RESULT([yes])
	AC_LIBGADU_DEFINE([HAVE_VA_COPY])
], [
	AC_MSG_RESULT([no])
])

AC_MSG_CHECKING([for __va_copy])
AC_TRY_LINK([#include <stdarg.h>], [va_list a, b; __va_copy(a, b);], 
[
	AC_MSG_RESULT([yes])
	AC_LIBGADU_DEFINE([HAVE___VA_COPY])
], [
	AC_MSG_RESULT([no])
])

AC_ARG_ENABLE(debug, 
	[  --disable-debug         Compile without debugging support])

if test "x$enable_debug" = "xno"; then
	AC_MSG_WARN([--disable-debug is obsolete.])
fi

dnl if test "x$enable_debug" = "xno"; then 
dnl 	CFLAGS="$CFLAGS -DGG_DEBUG_DISABLE"
dnl 	CFLAGS_LIBGADU="$CFLAGS_LIBGADU -DGG_DEBUG_DISABLE"
dnl 	strip_ekg="strip-ekg"
dnl 	strip_ioctld="strip-ioctld"
dnl 	strip_libgadu_so="strip-libgadu-so"
dnl else
dnl 	CFLAGS="$CFLAGS -g" 
dnl fi

dnl 
dnl  Jak± wersjê libgadu kompilujemy?
dnl

AC_ARG_ENABLE(shared,
	[  --enable-shared         Compile shared version of libgadu])

lib_shared_enabled=no
  
if test "x$enable_shared" = "xyes"; then
	shared="shared"
	install_shared="install-shared"
	lib_shared_enabled=yes
fi

AC_ARG_ENABLE(static,
	[  --disable-static        Don't compile static version of libgadu

Optional ekg Features:])

lib_static_enabled=yes

if test "x$enable_static" = "xno"; then
	static=""
	install_static=""
	lib_static_enabled=no
fi

if test "x$enable_static" = "xno" -a "x$enable_shared" != "xyes"; then
  AC_MSG_ERROR([You must choose either static or shared version of libgadu.])
fi

AC_ARG_ENABLE(dynamic,
  [  --enable-dynamic        Link ekg dynamically with libgadu])

dnl
dnl  Sprawd¼my, jak± wersjê vsnprintf() dostajemy. Dodatkowa opcja jest
dnl  dla crosskompilacji, bo nie mo¿na wtedy korzystaæ z AC_TRY_RUN().
dnl

AC_ARG_WITH(c99-vsnprintf,
  [  --with-c99-vsnprintf    Target system has C99-compatible vsnprintf()])

if test "x$with_c99_vsnprintf" = "xyes"; then
	AC_LIBGADU_DEFINE([HAVE_C99_VSNPRINTF])
fi

if test "x$with_c99_vsnprintf" = "x"; then
	AC_MSG_CHECKING([for C99-compatible vsnprintf()])
	AC_TRY_RUN(
	[
		#include <stdio.h>
		int main()
		{
			char tmp;
			return (snprintf(&tmp, sizeof(tmp), "test") != 4);
		}
	], [
		AC_MSG_RESULT([yes])
		AC_LIBGADU_DEFINE([HAVE_C99_VSNPRINTF])
	], [
		AC_MSG_RESULT([no])
	])
fi

dnl
dnl  Resolver libgadu oparty na pthread
dnl

AC_ARG_WITH(pthread,
  [  --with-pthread          Use pthread in resolver])

if test "x$with_pthread" = "xyes"; then
	ACX_PTHREAD(
	[
	 	dnl workaround dla pkconfiga
		if test "x$PTHREAD_CFLAGS" = "x-pthread"; then
			PTHREAD_LIBS="$PTHREAD_LIBS -pthread"
		fi

		LIBS="$PTHREAD_LIBS $LIBS"
		CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
		CFLAGS_LIBGADU="$CFLAGS_LIBGADU $PTHREAD_CFLAGS"
		CC="$PTHREAD_CC"
		have_pthread=yes

		AC_LIBGADU_DEFINE([HAVE_PTHREAD])
	], [
		AC_MSG_ERROR([Your system is not supporting pthreads])
	])
fi

dnl
dnl  Sprawdzamy, czy jest ,,long long'' na potrzeby obliczeñ czasów DCC
dnl

AC_MSG_CHECKING([for long long])

AC_TRY_COMPILE([],
[
	long long a = 1LL;
	unsigned long long b = 1LL;
], [
	AC_MSG_RESULT([yes])
	AC_LIBGADU_DEFINE([HAVE_LONG_LONG])
])

dnl
dnl  Sprawdzamy OpenSSL
dnl

AC_CHECK_OPENSSL

if test "x$have_openssl" = "xyes"; then
	OBJS="$OBJS simlite.o"
	LDFLAGS="$OPENSSL_LIBS $LDFLAGS"
	LIBS_LIBGADU="$LIBS_LIBGADU $OPENSSL_LIBS"
	CFLAGS="$CFLAGS $OPENSSL_INCLUDES"
	CFLAGS_LIBGADU="$CFLAGS_LIBGADU $OPENSSL_INCLUDES"
	PC_REQUIRES="$PC_REQUIRES openssl"
	AC_LIBGADU_DEFINE([HAVE_OPENSSL])
fi

dnl
dnl  Zachowujemy opcje dla libgadu
dnl

LIBS_LIBGADU="$LIBS_LIBGADU $LIBS"


AC_CONFIG_FILES(Makefile lib/Makefile lib/libgadu.pc)

AC_OUTPUT

echo
echo "configured options:"

if test "x$lib_static_enabled" = "xyes"; then
	if test "x$lib_static_default" = "xyes"; then
		echo " - static libgadu: enabled (default)"
	else
		echo " - static libgadu: enabled"
	fi
else
	echo " - static libgadu: disabled"
fi

if test "x$lib_shared_enabled" = "xyes"; then
	if test "x$lib_shared_default" = "xyes"; then
		echo " - shared libgadu: enabled (default)"
	else
		echo " - shared libgadu: enabled"
	fi
else
	echo " - shared libgadu: disabled"
fi


