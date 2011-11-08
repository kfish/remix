
INCLUDES = -I$(top_srcdir)/src/ctxdata -I$(top_srcdir)/include -I../ctxdata

if BUILD_MONITOR
monitor_programs = noisedemo squaredemo
if HAVE_LIBSNDFILE1
sndfile_programs = sndfiledemo
endif

endif

noinst_PROGRAMS = cd_list_test $(monitor_programs) $(sndfile_programs)

EXTRA_DIST = 909_cl.wav 1052.wav

REMIX_LIBS = ${top_builddir}/src/ctxdata/libctxdata.la ${top_builddir}/src/libremix/libremix.la -ldl

cd_list_test_SOURCES = cd_list_test.c
cd_list_test_LDADD = ../ctxdata/libctxdata.la

noisedemo_SOURCES = noisedemo.c
noisedemo_LDADD = $(REMIX_LIBS)

squaredemo_SOURCES = squaredemo.c
squaredemo_LDADD = $(REMIX_LIBS)

sndfiledemo_CFLAGS = -DSAMPLEDIR=\"$(srcdir)\"
sndfiledemo_SOURCES = sndfiledemo.c
sndfiledemo_LDADD = $(REMIX_LIBS) @SNDFILE_LIBS@
