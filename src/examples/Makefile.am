
INCLUDES = -I$(top_srcdir)/ctxdata -I$(top_srcdir)/include
noinst_PROGRAMS = cd_list_test noisedemo squaredemo sndfiledemo

EXTRA_DIST = 909_cl.wav 1052.wav

REMIX_LIBS = ../ctxdata/libctxdata.la ../libremix/libremix.la -ldl

cd_list_test_SOURCES = cd_list_test.c
cd_list_test_LDADD = ../ctxdata/libctxdata.la

noisedemo_SOURCES = noisedemo.c
noisedemo_LDADD = $(REMIX_LIBS) @ALSA_LIBS@

squaredemo_SOURCES = squaredemo.c
squaredemo_LDADD = $(REMIX_LIBS) @ALSA_LIBS@

sndfiledemo_SOURCES = sndfiledemo.c
sndfiledemo_LDADD = $(REMIX_LIBS) @SNDFILE_LIBS@ @ALSA_LIBS@
