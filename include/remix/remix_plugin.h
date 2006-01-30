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
 * remix_plugin.h -- libremix internal data types and functions.
 *
 * Conrad Parker <Conrad.Parker@CSIRO.AU>, August 2001
 */

#ifndef __REMIX_PLUGIN_H__
#define __REMIX_PLUGIN_H__

/*#define DEBUG*/

#if defined(__REMIX_PLUGIN__) || defined(__REMIX__)

#include <stdio.h>

#include "ctxdata.h"

#define REMIX_PLUGIN_API_MAJOR 1
#define REMIX_PLUGIN_API_MINOR 0
#define REMIX_PLUGIN_API_REVISION 0

typedef struct _RemixMetaAuthor RemixMetaAuthor;
typedef struct _RemixMetaText RemixMetaText;

typedef struct _RemixParameterScheme RemixParameterScheme;
typedef struct _RemixParameterRange RemixParameterRange;

typedef struct _RemixNamedParameter RemixNamedParameter;

typedef union _RemixConstraint RemixConstraint;

typedef struct _RemixMethods RemixMethods;;

#define REMIX_PLUGIN_WRITEABLE 1<<0
#define REMIX_PLUGIN_SEEKABLE  1<<1
#define REMIX_PLUGIN_CACHEABLE 1<<2
#define REMIX_PLUGIN_CAUSAL    1<<3

/* A base of a plugin */
typedef struct _RemixPlugin RemixPlugin;
/*typedef struct _RemixBase * RemixBase;*/

typedef struct _RemixChunk RemixChunk;

#if defined (__REMIX__)
#include "remix_private.h"
#else
typedef RemixOpaque RemixEnv;
typedef RemixOpaque RemixPoint;
typedef RemixOpaque RemixEnvelope;
typedef RemixOpaque RemixStream;
typedef RemixOpaque RemixChannel;
typedef RemixOpaque RemixDeck;
typedef RemixOpaque RemixTrack;
typedef RemixOpaque RemixLayer;
typedef RemixOpaque RemixSound;
typedef RemixOpaque RemixSquareTone;
typedef RemixOpaque RemixMonitor;
#endif


typedef CDList * (*RemixPluginInitFunc) (RemixEnv * env);
typedef int (*RemixPluginDestroyFunc) (RemixEnv * env, RemixPlugin * plugin);

typedef RemixBase * (*RemixInitFunc) (RemixEnv * env, RemixBase * base,
				      CDSet * parameters);

typedef CDSet * (*RemixSuggestFunc) (RemixEnv * env, RemixPlugin * plugin,
				     CDSet * parameters,
				     void * plugin_data);

typedef RemixBase * (*RemixCloneFunc) (RemixEnv * env, RemixBase * base);
typedef int (*RemixDestroyFunc) (RemixEnv * env, RemixBase * base);
typedef int (*RemixReadyFunc) (RemixEnv * env, RemixBase * base);
typedef RemixBase * (*RemixPrepareFunc) (RemixEnv * env, RemixBase * base);
typedef RemixCount (*RemixSeekFunc) (RemixEnv * env, RemixBase * base,
				     RemixCount count);
typedef RemixCount (*RemixLengthFunc) (RemixEnv * env, RemixBase * base);
typedef RemixCount (*RemixProcessFunc) (RemixEnv * env, RemixBase * base,
					RemixCount count,
					RemixStream * input,
					RemixStream * output);
typedef int (*RemixFlushFunc) (RemixEnv * env, RemixBase * base);

#define REMIX_FLAGS_NONE (0)

#define REMIX_AUTHOR(n,e) ((struct _RemixMetaAuthor){(n),(e)})
#define REMIX_ONE_AUTHOR(n,e) CD_SINGLETON_LIST(CD_POINTER(&(REMIX_AUTHOR((n),(e)))))

/* ChunkChunkFuncs and all the rest */

/* RemixChunkFunc: a function to apply to one chunk */
typedef RemixCount (*RemixChunkFunc) (RemixEnv * env, RemixChunk * chunk,
				      RemixCount offset,
				      RemixCount count, int channelname,
				      void * data);

/* RemixChunkChunkFunc: a function to apply between two chunks :) */
typedef RemixCount (*RemixChunkChunkFunc) (RemixEnv * env,
					   RemixChunk * src,
					   RemixCount src_offset,
					   RemixChunk * dest,
					   RemixCount dest_offset,
					   RemixCount count, int channelname,
					   void * data);

/* RemixChunkChunkChunkFunc: a function to apply between THREE chunks !! */
typedef RemixCount (*RemixChunkChunkChunkFunc) (RemixEnv * env,
						RemixChunk * src1,
						RemixCount src1_offset,
						RemixChunk * src2,
						RemixCount src2_offset,
						RemixChunk * dest,
						RemixCount dest_offset,
						RemixCount count,
						int channelname,
						void * data);

struct _RemixMetaAuthor {
  char * name;
  char * email;
};

struct _RemixMetaText {
  char * identifier;
  char * category;
  char * description;
  char * copyright;
  char * url;
  CDList * authors;
};

struct _RemixParameterRange {
  RemixFlags valid_mask;
  RemixParameter lower;
  RemixParameter upper;
  RemixParameter step;
};

struct _RemixNamedParameter {
  char * name;
  RemixParameter parameter;
};

#define REMIX_NAMED_PARAMETER(n,p) (&((struct _RemixNamedParameter){(n),(p)}))

union _RemixConstraint {
  CDList * list; /* list of RemixNamedParameter */
  RemixParameterRange * range;
};

struct _RemixParameterScheme {
  char * name;
  char * description;
  RemixParameterType type;
  RemixConstraintType constraint_type;
  RemixConstraint constraint;
  RemixFlags hints;
};

struct _RemixPlugin {
  RemixMetaText * metatext;
  RemixFlags flags;
  CDSet * init_scheme;
  RemixInitFunc init;
  CDSet * process_scheme;
  RemixSuggestFunc suggest;
  void * plugin_data;
  RemixPluginDestroyFunc destroy;
};

struct _RemixMethods {
  RemixCloneFunc clone;
  RemixDestroyFunc destroy;
  RemixReadyFunc ready;
  RemixPrepareFunc prepare;
  RemixProcessFunc process;
  RemixLengthFunc length;
  RemixSeekFunc seek;
  RemixFlushFunc flush;
};


struct _RemixChunk {
  RemixCount start_index;
  RemixCount length;
  RemixPCM * data;
};


/* debug */
void remix_dprintf (const char * fmt, ...);

/* SOUNDRENDER, remix_context */

RemixError remix_set_error (RemixEnv * env, RemixError error);


/* remix_base */
RemixBase * remix_base_new (RemixEnv * env);

RemixPlugin * remix_base_set_plugin (RemixEnv * env, RemixBase * base,
				     RemixPlugin * plugin);
RemixPlugin * remix_base_get_plugin (RemixEnv * env, RemixBase * base);
RemixMethods * remix_base_set_methods (RemixEnv * env, RemixBase * base,
				       RemixMethods * methods);
void * remix_base_set_instance_data (RemixEnv * env, RemixBase * base,
				     void * data);
void * remix_base_get_instance_data (RemixEnv * env, RemixBase * base);

RemixCount remix_base_get_mixlength (RemixEnv * env, RemixBase * base);
RemixSamplerate remix_base_get_samplerate (RemixEnv * env, RemixBase * base);
RemixTempo remix_base_get_tempo (RemixEnv * env, RemixBase * base);
CDSet * remix_base_get_channels (RemixEnv * env, RemixBase * base);

int remix_base_has_samplerate (RemixEnv * env, RemixBase * base);
int remix_base_has_tempo (RemixEnv * env, RemixBase * base);
int remix_base_encompasses_mixlength (RemixEnv * env, RemixBase * base);
int remix_base_encompasses_channels (RemixEnv * env, RemixBase * base);


/* remix_meta */
RemixMetaText * remix_meta_text_new (RemixEnv * env);
void remix_meta_text_free (RemixEnv * env, RemixMetaText * mt);
RemixMetaText * remix_get_meta_text (RemixEnv * env, RemixBase * base);
RemixMetaText * remix_set_meta_text (RemixEnv * env, RemixBase * base,
				     RemixMetaText * mt);


/* remix_null */
RemixCount remix_null_length (RemixEnv * env, RemixBase * base);
RemixCount remix_null_process (RemixEnv * env, RemixBase * base,
			       RemixCount count,
			       RemixStream * input, RemixStream * output);
RemixCount remix_null_seek (RemixEnv * env, RemixBase * base,
			    RemixCount offset);


/* remix_stream */
RemixCount remix_stream_chunkfuncify (RemixEnv * env, RemixStream * stream,
				      RemixCount count,
				      RemixChunkFunc func, void * data);
RemixCount remix_stream_chunkchunkfuncify (RemixEnv * env,
					   RemixStream * src,
					   RemixStream * dest,
					   RemixCount count,
					   RemixChunkChunkFunc func,
					   void * data);
RemixCount remix_stream_chunkchunkchunkfuncify (RemixEnv * env,
						RemixStream * src1,
						RemixStream * src2,
						RemixStream * dest,
						RemixCount count,
						RemixChunkChunkChunkFunc func,
						void * data);

/* RemixChannel */
RemixChunk * remix_channel_get_chunk_at (RemixEnv * env,
					 RemixChannel * channel,
					 RemixCount offset);

RemixCount remix_channel_chunkfuncify (RemixEnv * env, RemixChannel * channel,
				       RemixCount count, RemixChunkFunc func,
				       int channelname, void * data);
RemixCount remix_channel_chunkchunkfuncify (RemixEnv * env,
					    RemixChannel * src,
					    RemixChannel * dest,
					    RemixCount count,
					    RemixChunkChunkFunc func,
					    int channelname, void * data);
RemixCount remix_channel_chunkchunkchunkfuncify (RemixEnv * env,
						 RemixChannel * src1,
						 RemixChannel * src2,
						 RemixChannel * dest,
						 RemixCount count,
						 RemixChunkChunkChunkFunc func,
						 int channelname, void * data);
/* RemixPCM */

RemixCount _remix_pcm_clear_region (RemixPCM * data, RemixCount count,
				    void * unused);
RemixCount _remix_pcm_set (RemixPCM * data, RemixPCM value, RemixCount count);
RemixCount _remix_pcm_gain (RemixPCM * data, RemixCount count,
			    /* (RemixPCM *) */ void * gain);
RemixCount _remix_pcm_copy (RemixPCM * src, RemixPCM * dest, RemixCount count,
			    void * unused);
RemixCount _remix_pcm_add (RemixPCM * src, RemixPCM * dest, RemixCount count,
			   void * unused);
RemixCount _remix_pcm_mult (RemixPCM * src, RemixPCM * dest, RemixCount count,
			    void * unused);
RemixCount _remix_pcm_fade (RemixPCM * src, RemixPCM * dest, RemixCount count,
			    void * unused);
RemixCount _remix_pcm_interleave_2 (RemixPCM * src1, RemixPCM * src2, 
				    RemixCount count, void * data);
RemixCount _remix_pcm_deinterleave_2 (RemixPCM * dest1, RemixPCM * dest2,
				      RemixCount count, void * data);
RemixCount _remix_pcm_blend (RemixPCM * src, RemixPCM * blend, RemixPCM * dest,
			     RemixCount count, void * unused);
RemixCount _remix_pcm_write_linear (RemixPCM * data, RemixCount x1,
				    RemixPCM y1, RemixCount x2, RemixPCM y2,
				    RemixCount offset, RemixCount count);

#endif /* defined(__REMIX__) */

#endif /* __REMIX_PLUGIN_H__ */
