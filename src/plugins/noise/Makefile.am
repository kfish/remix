## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/include -I$(CTXDATADIR)

libdir = $(PACKAGE_PLUGIN_DIR)

CTXDATADIR = $(top_srcdir)/src/ctxdata
CTXDATA_LIBS = $(top_builddir)/src/ctxdata/libctxdata.la

REMIXDIR = ../../libremix
REMIX_LIBS = $(REMIXDIR)/libremix.la

lib_LTLIBRARIES = libremix_noise.la

libremix_noise_la_SOURCES = remix_noise.c
libremix_noise_la_LDFLAGS = -module -version-info 1:0:0
libremix_noise_la_LIBADD = $(REMIX_LIBS) $(CTXDATA_LIBS)
