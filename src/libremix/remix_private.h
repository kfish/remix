/*
 * libremix -- An audio mixing and sequencing library.
 *
 * Copyright (C) 2001 Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO), Australia.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * remix_private.h -- libremix internal data types and functions.
 *
 * Conrad Parker <conrad@metadecks.org>, August 2001
 */

#ifndef __REMIX_PRIVATE_H__
#define __REMIX_PRIVATE_H__

/*#define DEBUG*/

#if defined(__REMIX__)

#include <stdlib.h>
#include <stdio.h>

#include "ctxdata.h"

#include "remix_compat.h"
#include "remix_plugin.h"

/* Max line length for error messages etc. */
#define REMIX_MAXLINE 4096

#define REMIX_DEFAULT_MIXLENGTH 1024
#define REMIX_DEFAULT_SAMPLERATE 44100
#define REMIX_DEFAULT_TEMPO 120

typedef struct _RemixThreadContext RemixThreadContext;
typedef struct _RemixWorld RemixWorld;
typedef struct _RemixContext RemixContext;

typedef RemixThreadContext RemixEnv;

typedef struct _RemixPoint RemixPoint;
typedef struct _RemixEnvelope RemixEnvelope;
/*typedef struct _RemixChunk RemixChunk;*/
typedef struct _RemixChannel RemixChannel;
typedef struct _RemixStream RemixStream;
typedef struct _RemixDeck RemixDeck;
typedef struct _RemixTrack RemixTrack;
typedef struct _RemixLayer RemixLayer;
typedef struct _RemixSound RemixSound;


struct _RemixThreadContext {
  RemixError last_error;
  RemixContext * context;
  RemixWorld * world;
};

struct _RemixWorld {
  RemixCount refcount;
  CDList * plugins;
  CDList * bases;
  int purging;
};

struct _RemixContext {
  RemixSamplerate samplerate;
  RemixTempo tempo;
  CDSet * channels;
  RemixCount mixlength;
};

struct _RemixBase {
  RemixPlugin * plugin;
  RemixMethods * methods;
  CDSet * parameters;
  RemixCount offset; /* current position */
  RemixContext context_limit;
  void * instance_data;
};

struct _RemixPoint {
  RemixTime time;
  RemixPCM value;
};

struct _RemixEnvelope {
  RemixBase base;
  RemixEnvelopeType type;
  RemixTimeType timetype;
  CDList * points;
  CDList * _current_point_item;
  RemixCount _current_offset;
};

/* XXX: multichannel envelopes ? */


struct _RemixStream {
  RemixBase base;
  CDSet * channels;
};

struct _RemixChannel {
  CDList * chunks;
  RemixCount _current_offset;
  CDList * _current_chunk;
};

struct _RemixDeck {
  RemixBase base;
  CDList * tracks;
  RemixStream * _mixstream;
};

struct _RemixTrack {
  RemixBase base;
  RemixDeck * deck;
  RemixPCM gain;
  CDList * layers;
  RemixStream * _mixstream_a;
  RemixStream * _mixstream_b;
};

struct _RemixLayer {
  RemixBase base;
  RemixTrack * track;
  RemixTimeType timetype;
  CDList * sounds;
  /*RemixTime _current_time;*/
  CDList * _current_sound_item;
  RemixTempo _current_tempo;
  RemixCount _current_offset;
};

struct _RemixSound {
  RemixBase base;
  RemixBase * source;
  RemixBase * rate_envelope;
  RemixBase * gain_envelope;
  RemixBase * blend_envelope;
  RemixLayer * layer;
  RemixTime start_time; /* position in layer */
  RemixTime duration; /* maximum time length */
  RemixCount cutin;  /* start offset into sound source */
  RemixCount cutlength;
  RemixCount _current_source_offset;
  RemixStream * _rate_envstream;
  RemixStream * _gain_envstream;
  RemixStream * _blend_envstream;
};

typedef struct _RemixMonitor RemixMonitor;

#define REMIX_MONITOR_BUFFERLEN 2048

struct _RemixMonitor {
  RemixBase base;
  RemixPCM databuffer[REMIX_MONITOR_BUFFERLEN];
  short playbuffer[REMIX_MONITOR_BUFFERLEN];
  int dev_dsp_fd;
  int mode;
  int mask;
  int format;
  int stereo;
  int frequency;
  int numfrags;
  int fragsize;
};

#define _remix_time_zero(t) (RemixTime)\
  (((t)==REMIX_TIME_SAMPLES) ? ((RemixCount)0) : \
  (((t)==REMIX_TIME_SECONDS) ? ((float)0.0) : \
                            ((int)0)))

#define _remix_time_invalid(t) (RemixTime)\
  (((t)==REMIX_TIME_SAMPLES) ? ((RemixCount)-1) : \
  (((t)==REMIX_TIME_SECONDS) ? ((float)-1.0) : \
                            ((int)-1)))

#define _remix_time_is_invalid(t,ti) \
  (((t)==REMIX_TIME_SAMPLES) ? ((ti).samples < (RemixCount)0) : \
  (((t)==REMIX_TIME_SECONDS) ? ((ti).seconds < (float)0.0) : \
                            ((ti).beat24s < (int)0)))

#define _remix_time_add(t,t1,t2) \
  (((t)==REMIX_TIME_SAMPLES) ? REMIX_SAMPLES((t1).samples + (t2).samples) : \
  (((t)==REMIX_TIME_SECONDS) ? REMIX_SECONDS((t1).seconds + (t2).seconds) : \
                            REMIX_BEAT24S((t1).beat24s + (t2).beat24s)))

#define _remix_time_sub(t,t1,t2) \
  (((t)==REMIX_TIME_SAMPLES) ? REMIX_SAMPLES((t1).samples - (t2).samples) : \
  (((t)==REMIX_TIME_SECONDS) ? REMIX_SECONDS((t1).seconds - (t2).seconds) : \
                            REMIX_BEAT24S((t1).beat24s - (t2).beat24s)))

#define _remix_time_eq(t,t1,t2) \
  (((t)==REMIX_TIME_SAMPLES) ? ((t1).samples == (t2).samples) : \
  (((t)==REMIX_TIME_SECONDS) ? ((t1).seconds == (t2).seconds) : \
                            ((t1).beat24s == (t2).beat24s)))

#define _remix_time_gt(t,t1,t2) \
  (((t)==REMIX_TIME_SAMPLES) ? ((t1).samples > (t2).samples) : \
  (((t)==REMIX_TIME_SECONDS) ? ((t1).seconds > (t2).seconds) : \
                            ((t1).beat24s > (t2).beat24s)))

#define _remix_time_lt(t,t1,t2) \
  (((t)==REMIX_TIME_SAMPLES) ? ((t1).samples < (t2).samples) : \
  (((t)==REMIX_TIME_SECONDS) ? ((t1).seconds < (t2).seconds) : \
                            ((t1).beat24s < (t2).beat24s)))

#define _remix_time_ge(t,t1,t2) \
  (((t)==REMIX_TIME_SAMPLES) ? ((t1).samples >= (t2).samples) : \
  (((t)==REMIX_TIME_SECONDS) ? ((t1).seconds >= (t2).seconds) : \
                            ((t1).beat24s >= (t2).beat24s)))

#define _remix_time_le(t,t1,t2) \
  (((t)==REMIX_TIME_SAMPLES) ? ((t1).samples <= (t2).samples) : \
  (((t)==REMIX_TIME_SECONDS) ? ((t1).seconds <= (t2).seconds) : \
                            ((t1).beat24s <= (t2).beat24s)))

#define _remix_time_min(t,t1,t2) \
  (((t)==REMIX_TIME_SAMPLES) ? ((t1).samples < (t2).samples ? (t1) : (t2)) : \
  (((t)==REMIX_TIME_SECONDS) ? ((t1).seconds < (t2).seconds ? (t1) : (t2)) : \
                            ((t1).beat24s < (t2).beat24s ? (t1) : (t2))))

#define _remix_time_max(t,t1,t2) \
  (((t)==REMIX_TIME_SAMPLES) ? ((t1).samples > (t2).samples ? (t1) : (t2)) : \
  (((t)==REMIX_TIME_SECONDS) ? ((t1).seconds > (t2).seconds ? (t1) : (t2)) : \
                            ((t1).beat24s > (t2).beat24s ? (t1) : (t2))))

#define _remix_base_get_samplerate(a,b) (((RemixBase*)b)->context_limit.samplerate)
#define _remix_base_get_tempo(a,b) (((RemixBase*)b)->context_limit.tempo)
#define _remix_base_get_mixlength(a,b) (((RemixBase*)b)->context_limit.mixlength)
#define _remix_base_get_channels(a,b) (((RemixBase*)b)->context_limit.channels)

#define _remix_set_plugin(a,b,p) (((RemixBase*)b)->plugin = (p))
#define _remix_get_plugin(a,b) (((RemixBase*)b)->plugin)
#define _remix_set_methods(a,b,m) (((RemixBase*)b)->methods = (m))
#define _remix_get_methods(a,b) (((RemixBase*)b)->methods)
#define _remix_set_instance_data(a,b,d) (((RemixBase*)b)->instance_data = (d))
#define _remix_get_instance_data(a,b) (((RemixBase*)b)->instance_data)
#define _remix_set_name(a,b,n) (((RemixBase*)b)->name = (n))
#define _remix_get_name(a,b) (((RemixBase*)b)->name)
#define _remix_clone(a,b) (((RemixBase*)b)->methods->clone ((a), ((RemixBase*)b)))
#define _remix_destroy(a,b) (((RemixBase*)b)->methods->destroy ((a), ((RemixBase*)b)))
#define _remix_prepare(a,b) (((RemixBase*)b)->methods->prepare ((a), ((RemixBase*)b)))
#define _remix_process(a,b,c,i,o) \
        (((RemixBase*)b)->methods->process ((a),((RemixBase*)b),(c),(i),(o)))
#define _remix_length(a,b) (((RemixBase*)b)->methods->length ((a), ((RemixBase*)b)))
#define _remix_flush(a,b) (((RemixBase*)b)->methods->flush ((a), ((RemixBase*)b)))


/* util */
#define remix_malloc(x) calloc(1, x)
#define remix_free free

/* debug */
void remix_debug_down (void);
void remix_debug_up (void);

/* RemixEnv, remix_context */

RemixBase * remix_base_new_subclass (RemixEnv * env, size_t size);

RemixContext * _remix_context_copy (RemixEnv * env, RemixContext * dest);
RemixContext * _remix_context_merge (RemixEnv * env, RemixContext * dest);
RemixEnv * _remix_register_plugin (RemixEnv * env, RemixPlugin * plugin);
RemixEnv * _remix_unregister_plugin (RemixEnv * env, RemixPlugin * plugin);
RemixEnv * _remix_register_base (RemixEnv * env, RemixBase * base);
RemixEnv * _remix_unregister_base (RemixEnv * env, RemixBase * base);

/* remix_plugin */
void remix_plugin_defaults_initialise (RemixEnv * env);

/* remix_deck */
RemixTrack * _remix_deck_add_track (RemixEnv * env, RemixDeck * deck,
				    RemixTrack * track);
RemixTrack * _remix_deck_remove_track (RemixEnv * env, RemixDeck * deck,
				       RemixTrack * track);

/* remix_track */
RemixBase * remix_track_clone (RemixEnv * env, RemixBase * base);
RemixLayer * _remix_track_add_layer_above (RemixEnv * env, RemixTrack * track,
				     RemixLayer * layer, RemixLayer * above);
RemixLayer * _remix_track_remove_layer (RemixEnv * env, RemixTrack * track,
				  RemixLayer * layer);
RemixLayer * _remix_track_get_layer_above (RemixEnv * env, RemixTrack * track,
				     RemixLayer * above);
RemixLayer * _remix_track_get_layer_below (RemixEnv * env, RemixTrack * track,
				     RemixLayer * below);

/* remix_layer */
RemixLayer * _remix_remove_layer (RemixEnv * env, RemixLayer * layer);
RemixBase * remix_layer_clone (RemixEnv * env, RemixBase * base);
RemixSound * _remix_layer_add_sound (RemixEnv * env, RemixLayer * layer,
				     RemixSound * sound, RemixTime position);
RemixSound * _remix_layer_remove_sound (RemixEnv * env, RemixLayer * layer,
				  RemixSound * sound);
RemixSound * _remix_layer_get_sound_prev (RemixEnv * env, RemixLayer * layer,
				    RemixSound * sound);
RemixSound * _remix_layer_get_sound_next (RemixEnv * env, RemixLayer * layer,
				    RemixSound * sound);

/* remix_sound */
RemixBase *  remix_sound_clone_with_layer (RemixEnv * env, RemixBase * base,
					   RemixLayer * new_layer);
int remix_sound_later (RemixEnv * env, RemixSound * s1, RemixSound * s2);

/* remix_envelope */
RemixBase * remix_envelope_clone (RemixEnv * env, RemixBase * base);


/* remix_channel */
RemixChannel * remix_channel_new (RemixEnv * env);
RemixChannel * remix_channel_clone (RemixEnv * env, RemixChannel * channel);
int remix_channel_destroy (RemixEnv * env, RemixBase * base);

RemixChunk * remix_channel_add_chunk (RemixEnv * env, RemixChannel * channel,
				      RemixChunk * chunk);
RemixChunk * remix_channel_add_new_chunk (RemixEnv * env,
					  RemixChannel * channel,
					  RemixCount offset,
					  RemixCount length);

RemixCount remix_channel_write0 (RemixEnv * env, RemixChannel * channel,
				 RemixCount length);

RemixCount _remix_channel_write (RemixEnv * env, RemixChannel * channel,
				 RemixCount count, RemixChannel * data);
RemixCount _remix_channel_length (RemixEnv * env, RemixChannel * channel);
RemixCount _remix_channel_seek (RemixEnv * env, RemixChannel * channel,
				RemixCount offset);

RemixCount remix_channel_interleave_2 (RemixEnv * env,
				       RemixChannel * src1,
				       RemixChannel * src2,
				       RemixPCM * dest, RemixCount count);
RemixCount remix_channel_deinterleave_2 (RemixEnv * env,
					 RemixChannel * dest1,
					 RemixChannel * dest2,
					 RemixPCM * src, RemixCount count);
RemixCount remix_channel_mix (RemixEnv * env, RemixChannel * src,
			      RemixChannel * dest, RemixCount count);


/* remix_channelset */
void remix_channelset_defaults_initialise (RemixEnv * env);
void remix_channelset_defaults_destroy (RemixEnv * env);

/* remix_chunk */
RemixChunk * remix_chunk_new (RemixEnv * env, RemixCount start_index,
			      RemixCount length);
RemixChunk * remix_chunk_new_from_buffer (RemixEnv * env,
					  RemixCount start_index,
					  RemixCount length,
					  RemixPCM * buffer);
RemixChunk * remix_chunk_clone (RemixEnv * env, RemixChunk * chunk);
void remix_chunk_free (RemixEnv * env, RemixChunk * chunk);
RemixCount _remix_chunk_clear_region (RemixEnv * env, RemixChunk * chunk,
				      RemixCount start, RemixCount length,
				      int channelname, void * unused);
RemixCount _remix_chunk_gain (RemixEnv * env, RemixChunk * chunk,
			      RemixCount start, RemixCount count,
			      int channelname, /* (RemixPCM *) */ void * gain);
RemixCount _remix_chunk_copy (RemixEnv * env, RemixChunk * src,
			      RemixCount src_offset,
			      RemixChunk * dest, RemixCount dest_offset,
			      RemixCount count, int channelname,
			      void * unused);
RemixCount _remix_chunk_add_inplace (RemixEnv * env, RemixChunk * src,
				     RemixCount src_offset,
				     RemixChunk * dest, RemixCount dest_offset,
				     RemixCount count, int channelname,
				     void * unused);
RemixCount _remix_chunk_mult_inplace (RemixEnv * env, RemixChunk * src,
				      RemixCount src_offset,
				      RemixChunk * dest,
				      RemixCount dest_offset,
				      RemixCount count, int channelname,
				      void * unused);
RemixCount _remix_chunk_fade_inplace (RemixEnv * env, RemixChunk * src,
				      RemixCount src_offset,
				      RemixChunk * dest,
				      RemixCount dest_offset,
				      RemixCount count, int channelname,
				      void * unused);
RemixCount _remix_chunk_interleave_2 (RemixEnv * env,
				      RemixChunk * src1,
				      RemixCount src1_offset,
				      RemixChunk * src2,
				      RemixCount src2_offset,
				      RemixCount count,
				      int unused, void * dest);
RemixCount _remix_chunk_deinterleave_2 (RemixEnv * env,
					RemixChunk * dest1,
					RemixCount dest1_offset,
					RemixChunk * dest2,
					RemixCount dest2_offset,
					RemixCount count,
					int unused, void * src);
RemixCount _remix_chunk_blend_inplace (RemixEnv * env,
				       RemixChunk * src, RemixCount src_offset,
				       RemixChunk * blend,
				       RemixCount blend_offset,
				       RemixChunk * dest,
				       RemixCount dest_offset,
				       RemixCount count,
				       int channelname, void * unused);

/* XXX: remove these when dynamic! */
CDList * __gain_init (RemixEnv * env);
CDList * __sndfile_init (RemixEnv * env);

#endif /* defined(__REMIX__) */

#endif /* __REMIX_PRIVATE_H__ */
