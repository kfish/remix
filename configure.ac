dnl Process this file with autoconf to produce a configure script.
AC_INIT(src/libremix/remix_base.c)

AM_INIT_AUTOMAKE(remix, 0.2.3)
AM_CONFIG_HEADER(config.h)

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
else
  AC_CHECK_LIB(sndfile, sf_open, SNDFILE_LIBS="maybe")
  if test "$SNDFILE_LIBS" = "maybe" ; then
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


dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(limits.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_UID_T

dnl Checks for library functions.
AC_CHECK_FUNCS(strdup strerror)





dnl Test for ALSA.

HAVE_ALSA=0
AC_CHECK_HEADERS(alsa/asoundlib.h)
  if test "x${ac_cv_header_alsa_asoundlib_h}" = xyes ; then
    HAVE_ALSA=1
    ALSA_LIBS="-lasound"
    AC_SUBST(ALSA_LIBS)
  fi

AC_DEFINE_UNQUOTED([HAVE_ALSA],${HAVE_ALSA}, [Set to 1 if using ALSA for sound output.])





dnl Set PACKAGE_PLUGIN_DIR in Makefiles and config.h.
PACKAGE_PLUGIN_DIR=$libdir/remix
AC_SUBST(PACKAGE_PLUGIN_DIR)
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(PACKAGE_PLUGIN_DIR, "${ac_default_prefix}/lib/${PACKAGE}",
    [Set the plugin directory for remix])
else
  AC_DEFINE_UNQUOTED(PACKAGE_PLUGIN_DIR, "${prefix}/lib/${PACKAGE}",
    [Set the plugin directory for remix])
fi

dnl Use -Wall if we have gcc.
changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[\ \ ]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi
changequote([,])dnl

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

  Example programs (./src/examples):

    noisedemo sndfiledemo squaredemo

  Installation paths:

    libremix: .................... ${prefix}/lib
    C header files: .............. ${prefix}/include/remix
    Documentation: ............... ${prefix}/share/doc/$PACKAGE

  Example programs will be built but not installed.
------------------------------------------------------------------------
])

