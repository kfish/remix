## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/include

libdir = $(PACKAGE_PLUGIN_DIR)

CTXDATADIR = ../../ctxdata
CTXDATA_LIBS = $(CTXDATADIR)/libctxdata.la

REMIXDIR = ../../libremix
REMIX_LIBS = $(REMIXDIR)/libremix.la

lib_LTLIBRARIES = \
	libremix_ladspa.la

libremix_ladspa_la_SOURCES = remix_ladspa.c ladspa.h
libremix_ladspa_la_LDFLAGS = -version-info 1:0:0
libremix_ladspa_la_LIBADD = $(REMIX_LIBS) $(CTXDATA_LIBS)
