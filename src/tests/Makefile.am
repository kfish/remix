noinst_PROGRAMS = sndfiletest

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/ctxdata

REMIX_LIBS = ../ctxdata/libctxdata.la ../libremix/libremix.la -ldl

TESTS = sndfiletest

sndfiletest_SOURCES = sndfiletest.c
sndfiletest_LDADD = $(REMIX_LIBS) @SNDFILE_LIBS@
