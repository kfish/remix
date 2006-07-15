dnl Process this file with autoconf to produce a configure script.
AC_INIT(src/libremix/remix_base.c)

AM_INIT_AUTOMAKE(remix, 0.2.3)
AM_CONFIG_HEADER(config.h)

example_programs="noisedemo squaredemo"

dnl Checks for programs.
AM_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Check for doxygen
AC_CHECK_PROG(HAVE_DOXYGEN, doxygen, true, false)
AM_CONDITIONAL(HAVE_DOXYGEN,$HAVE_DOXYGEN)
if test $HAVE_DOXYGEN = "false"; then
             AC_MSG_WARN([*** doxygen not found, docs will not be built])
fi

# Check for valgrind
VALGRIND_ENVIRONMENT=""
ac_enable_valgrind=no
AC_ARG_ENABLE(valgrind-testing,
     [  --enable-valgrind-testing     enable running of tests inside Valgrind ],     [ ac_enable_valgrind=yes ], [ ac_enable_valgrind=no] )

if test "x${ac_enable_valgrind}" = xyes ; then
  if test "x${enable_shared}" = xyes ; then
    VALGRIND_ENVIRONMENT="libtool --mode=execute "
  fi

  AC_CHECK_PROG(HAVE_VALGRIND, valgrind, yes, no)
  if test "x$HAVE_VALGRIND" = xyes ; then
    VALGRIND_ENVIRONMENT="$VALGRIND_ENVIRONMENT valgrind -q --leak-check=yes --show-reachable=yes --num-callers=100"
    AC_SUBST(VALGRIND_ENVIRONMENT)
    TESTS_INFO="'make check' test suite will be run under:
  ${VALGRIND_ENVIRONMENT}"
  else
    TESTS_INFO="'make check' to run test suite (Valgrind not found)"
  fi
else
  TESTS_INFO="'make check' to run test suite (Valgrind testing not enabled)"
fi

dnl
dnl  Configuration option for building of experimental code.
dnl

ac_enable_experimental=no
AC_ARG_ENABLE(experimental,
     [  --enable-experimental   enable building of experimental code ],
     [ ac_enable_experimental=yes ])

if test "x${ac_enable_experimental}" = xyes ; then
    AC_DEFINE(ANX_CONFIG_EXPERIMENTAL, [], [Define to build experimental code])
fi

dnl Checks for libraries.

dnl
dnl  Detect libsndfile 1.0
dnl


PKG_CHECK_MODULES(SNDFILE, sndfile >= 1.0.0,
                  HAVE_LIBSNDFILE1="yes", HAVE_LIBSNDFILE1="no")

if test "$HAVE_LIBSNDFILE1" = "yes" ; then
  AC_DEFINE([HAVE_LIBSNDFILE1], [], [Define if libsndfile version 1 found])
  AC_SUBST(SNDFILE_LIBS)
  AC_SUBST(SNDFILE_CFLAGS)
  example_programs="${example_programs} sndfiledemo"
else
  AC_CHECK_LIB(sndfile, sf_open, HAVE_LIBSNDFILE1="maybe")
  if test "$HAVE_LIBSNDFILE1" = "maybe" ; then
    AC_MSG_ERROR([
***
*** libsndfile-1.0.0 or greater seems to exist on your system, however
*** the pkg-config tool cannot find its build information.
***
*** Please set your PKG_CONFIG_PATH environment variable to include the
*** directory in which sndfile.pc was installed. For example, sndfile.pc
*** is installed in /usr/local/lib/pkgconfig by default; if that is the
*** case do:
***
***     export PKG_CONFIG_PATH="\$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig"
***
*** then run this ./configure again. If you configured libsndfile with a
*** different --prefix option, replace /usr/local above with that prefix.
***
])
  fi
fi

AM_CONDITIONAL(HAVE_LIBSNDFILE1, test "x${HAVE_LIBSNDFILE1}" = xyes)


dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(limits.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_UID_T

dnl Checks for library functions.
AC_CHECK_FUNCS(strdup strerror)

dnl Test for sys/soundcard.h -- if user doesn't have it, don't build remix_monitor
HAVE_SYS_SOUNDCARD_H=0
AC_CHECK_HEADERS(sys/soundcard.h)
  if test "x${ac_cv_header_sys_soundcard_h}" = xyes ; then
    HAVE_SYS_SOUNDCARD_H=1
    BUILD_MONITOR=1
  fi
AC_SUBST(BUILD_MONITOR)
AM_CONDITIONAL(BUILD_MONITOR, test -n "$BUILD_MONITOR")

dnl
dnl Set PACKAGE_PLUGIN_DIR in Makefiles and config.h.
dnl

PACKAGE_PLUGIN_DIR="${libdir}/remix"
AC_SUBST(PACKAGE_PLUGIN_DIR)
if test "x${prefix}" = "xNONE"; then
  PACKAGE_PLUGIN_DIR="${ac_default_prefix}/lib/remix"
else
  PACKAGE_PLUGIN_DIR="${prefix}/lib/remix"
fi
AC_DEFINE_UNQUOTED(PACKAGE_PLUGIN_DIR, "$PACKAGE_PLUGIN_DIR",
    [Set the plugin directory for remix])

AC_ARG_ENABLE(gcc-werror,
	AC_HELP_STRING([--enable-gcc-werror], [enable -Werror in all Makefiles]))

dnl Use -Wall if we have gcc.
if test "x$ac_cv_prog_gcc" = xyes ; then
  CFLAGS="$CFLAGS -Wall"

  if test x$enable_gcc_werror = "xyes" ; then
    CFLAGS="$CFLAGS -Werror"
	fi

fi

AC_OUTPUT([
Makefile
doc/Makefile
doc/Doxyfile
include/Makefile
include/remix/Makefile
src/Makefile
src/ctxdata/Makefile
src/libremix/Makefile
src/plugins/Makefile
src/plugins/ladspa/Makefile
src/plugins/noise/Makefile
src/examples/Makefile
src/tests/Makefile
ctxdata.pc
remix.pc
remix.spec
])

AC_MSG_RESULT([
------------------------------------------------------------------------
  $PACKAGE $VERSION:  Automatic configuration OK.

  General configuration:

    Experimental code: ........... ${ac_enable_experimental}

  Plugins:

    ladspa noise

  Example programs (./src/examples):

    ${example_programs}

  Installation paths:

    libremix: .................... ${prefix}/lib
    Plugins: ..................... ${PACKAGE_PLUGIN_DIR}
    C header files: .............. ${prefix}/include/remix
    Documentation: ............... ${prefix}/share/doc/$PACKAGE

  Example programs will be built but not installed.
------------------------------------------------------------------------
])

