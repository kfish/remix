INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/include/remix $(SNDFILE_CFLAGS) -I$(top_srcdir)/src/ctxdata

lib_LTLIBRARIES = libremix.la

if BUILD_MONITOR
monitor_sources = remix_monitor.c
endif

libremix_la_SOURCES = \
	$(monitor_sources) \
	remix_base.c \
	remix_channel.c \
	remix_channelset.c \
	remix_chunk.c \
	remix_context.c \
	remix_debug.c \
	remix_deck.c \
	remix_envelope.c \
	remix_error.c \
	remix_gain.c \
	remix_layer.c \
	remix_meta.c \
	remix_null.c \
	remix_pcm.c \
	remix_plugin.c \
	remix_sndfile.c \
	remix_sound.c \
	remix_squaretone.c \
	remix_stream.c \
	remix_time.c \
	remix_track.c \
	remix_private.h \
	remix_compat.h

libremix_la_LIBADD = @SNDFILE_LIBS@ @ALSA_LIBS@

