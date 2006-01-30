## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -pedantic

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/ctxdata

REMIX_LIBS = ../ctxdata/libctxdata.la ../libremix/libremix.la -ldl

# Test programs

TESTS_ENVIRONMENT = $(VALGRIND_ENVIRONMENT)

test: check

TESTS = noop sndfiletest

noinst_PROGRAMS = $(TESTS)
noinst_HEADERS = tests.h

noop_SOURCES = noop.c
noop_LDADD = $(REMIX_LIBS)

sndfiletest_SOURCES = sndfiletest.c
sndfiletest_LDADD = $(REMIX_LIBS) @SNDFILE_LIBS@
