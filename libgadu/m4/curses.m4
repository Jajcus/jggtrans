dnl Rewritten from scratch. --wojtekka
dnl $Id: curses.m4,v 1.17 2003/06/24 21:20:12 wojtekka Exp $

AC_DEFUN(AC_CHECK_NCURSES,
[
	AC_SUBST(CURSES_LIBS)
	AC_SUBST(CURSES_INCLUDES)

	AC_ARG_WITH(ncurses, [[  --with-ncurses[=dir]    Compile with ncurses/locate base dir]],
	[
		if test "x$withval" = "xno" ; then
			without_ncurses=yes
			with_arg=""
		elif test "x$withval" != "xyes" ; then
			with_arg=$withval/include:-L$withval/lib
		fi
	])

	if test "x$without_ncurses" != "xyes" ; then
		for i in $with_arg \
			: \
			-I/usr/pkg/include:-L/usr/pkg/lib \
			-I/usr/contrib/include:-L/usr/contrib/lib \
			-I/usr/freeware/include:-L/usr/freeware/lib32 \
			-I/sw/include:-L/sw/lib \
			-I/cw/include:-L/cw/lib \
			-I/boot/home/config/include:-L/boot/home/config/lib; do
	
			incl=`echo "$i" | sed 's/:.*//'`	# 'g³upi vim
			lib=`echo "$i" | sed 's/.*://'`		# 'g³upi vim
			path=`echo "$incl" | sed 's/^..//'`	# 'g³upi vim

			cppflags="$CPPFLAGS"
			ldflags="$LDFLAGS"

			CPPFLAGS="$CPPFLAGS $incl"
			LDFLAGS="$LDFLAGS $libs"

			have_ncurses_h=""

			if test "x$path" = "x" -o -f "$path/ncurses.h" -o -f "$path/ncurses/ncurses.h"; then
				$as_unset ac_cv_header_ncurses_h
				$as_unset ac_cv_header_ncurses_ncurses_h
				AC_CHECK_HEADERS([ncurses.h], [
					CURSES_INCLUDES="$incl"
					have_ncurses_h=yes
				], [
					AC_CHECK_HEADERS([ncurses/ncurses.h],
					[
						CURSES_INCLUDES="$incl"
						have_ncurses_h=yes
					])
				])
			fi

			if test "x$have_ncurses_h" = "xyes"; then
				$as_unset ac_cv_lib_ncurses_initscr
				$as_unset ac_cv_lib_curses_initscr
				have_libncurses=""
				AC_CHECK_LIB(ncurses, initscr,
				[
					CURSES_LIBS="$libs -lncurses"
					have_libncurses=yes
				], [
					AC_CHECK_LIB(curses, initscr,
					[
						CURSES_LIBS="$libs -lcurses"
						have_libncurses=yes
					])
				])

				if test "x$have_libncurses" = "xyes"; then
					AC_DEFINE(HAVE_NCURSES, 1, [define if ncurses is installed])
					have_ncurses=yes
					break
				fi
			fi

			CPPFLAGS="$cppflags"
			LDFLAGS="$ldflags"
		done
	fi
])


