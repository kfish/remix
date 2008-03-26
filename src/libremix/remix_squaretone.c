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
 * RemixSquareTone: a square wave tone generator
 *
 * Conrad Parker <conrad@metadecks.org>, August 2001
 */

#define __REMIX__
#include "remix.h"

typedef struct _RemixSquareToneChannel RemixSquareToneChannel;
typedef struct _RemixSquareTone RemixSquareTone;

struct _RemixSquareToneChannel {
  RemixCount _cycle_offset;
};

struct _RemixSquareTone {
  RemixBase base;
  float frequency;
  CDSet * channels;
};

/* Optimisation dependencies: none */
static RemixSquareTone * remix_squaretone_optimise (RemixEnv * env,
					      RemixSquareTone * squaretone);

static RemixSquareTone *
remix_squaretone_replace_channels (RemixEnv * env, RemixSquareTone * squaretone)
{
  RemixSquareToneChannel * sqch;
  CDSet * s, * channels = remix_get_channels (env);
  RemixCount offset = remix_tell (env, (RemixBase *)squaretone);

  cd_set_free_all (env, squaretone->channels);

  for (s = channels; s; s = s->next) {
    remix_dprintf ("[remix_squaretone_replace_channels] %p replacing channel %d\n",
		squaretone, s->key);
    sqch = (RemixSquareToneChannel *)
      remix_malloc (sizeof (struct _RemixSquareToneChannel));
    sqch->_cycle_offset = 0;
    squaretone->channels =
      cd_set_insert (env, squaretone->channels, s->key, CD_POINTER(sqch));
  }

  if (offset > 0) remix_seek (env, (RemixBase *)squaretone, offset, SEEK_SET);

  return squaretone;
}

static RemixBase *
remix_squaretone_init (RemixEnv * env, RemixBase * base)
{
  RemixSquareTone * squaretone = (RemixSquareTone *)base;
  squaretone->channels = NULL;
  remix_squaretone_replace_channels (env, squaretone);
  remix_squaretone_optimise (env, squaretone);
  return base;
}

RemixBase *
remix_squaretone_new (RemixEnv * env, float frequency)
{
  RemixSquareTone * squaretone = (RemixSquareTone *)
    remix_base_new_subclass (env, sizeof (struct _RemixSquareTone));
  remix_squaretone_init (env, (RemixBase *)squaretone);
  squaretone->frequency = frequency;

  return (RemixBase *)squaretone;
}

static RemixBase *
remix_squaretone_clone (RemixEnv * env, RemixBase * base)
{
  RemixSquareTone * squaretone = (RemixSquareTone *)base;
  RemixBase * new_squaretone =
    remix_squaretone_new (env, squaretone->frequency);
  remix_squaretone_optimise (env, (RemixSquareTone *)new_squaretone);
  return new_squaretone;
}

static int
remix_squaretone_destroy (RemixEnv * env, RemixBase * base)
{
  RemixSquareTone * squaretone = (RemixSquareTone *)base;
  remix_free (squaretone);
  return 0;
}

static int
remix_squaretone_ready (RemixEnv * env, RemixBase * base)
{
  return (remix_base_has_samplerate (env, base) &&
	  remix_base_encompasses_channels (env, base));
}

static RemixBase *
remix_squaretone_prepare (RemixEnv * env, RemixBase * base)
{
  RemixSquareTone * squaretone = (RemixSquareTone *)base;
  remix_squaretone_replace_channels (env, squaretone);
  return base;
}

float
remix_squaretone_set_frequency (RemixEnv * env, RemixBase * base,
			     float frequency)
{
  RemixSquareTone * squaretone = (RemixSquareTone *)base;
  float old = squaretone->frequency;
  squaretone->frequency = frequency;
  return old;
}

float
remix_squaretone_get_frequency (RemixEnv * env, RemixBase * base)
{
  RemixSquareTone * squaretone = (RemixSquareTone *)base;
  return squaretone->frequency;
}

/* An RemixChunkFunc for creating squaretone data */
static RemixCount
remix_squaretone_write_chunk (RemixEnv * env, RemixChunk * chunk,
                              RemixCount offset, RemixCount count,
                              int channelname, void * data)
{
  RemixSquareTone * squaretone = (RemixSquareTone *)data;
  RemixSquareToneChannel * sqch;
  RemixCount remaining = count, written = 0, n;
  RemixCount wavelength;
  RemixPCM * d, value;
  CDScalar k;

  remix_dprintf ("[remix_squaretone_write_chunk] (+%ld) @ %ld\n",
                 count, offset);

  k = cd_set_find (env, squaretone->channels, channelname);
  sqch = k.s_pointer;

  if (sqch == RemixNone) {
    remix_dprintf ("[remix_squaretone_write_chunk] channel %d not found\n",
                   channelname);
    remix_set_error (env, REMIX_ERROR_SILENCE);
    return -1;
  }

  wavelength = (RemixCount)(remix_get_samplerate (env) / squaretone->frequency);

  remix_dprintf ("[remix_squaretone_write_chunk] wavelength %ld, cycle_offset %ld\n",
                 wavelength, sqch->_cycle_offset);
  
  if (sqch->_cycle_offset < wavelength/2) {
    n = MIN (remaining, wavelength/2 - sqch->_cycle_offset);
    value = 1.0;
  } else {
    n = MIN (remaining, wavelength - sqch->_cycle_offset);
    value = -1.0;
  }

  d = &chunk->data[offset];
  _remix_pcm_set (d, value, n);
  remaining -= n; written += n; offset += n;

  while (remaining > 0) {
    n = MIN (remaining, wavelength/2);
    value = -(value);
    d = &chunk->data[offset];
    _remix_pcm_set (d, value, n);
    remaining -= n; written += n; offset += n;
  }

  sqch->_cycle_offset += written;
  sqch->_cycle_offset %= wavelength;

  remix_dprintf ("[remix_squaretone_write_chunk] written %ld\n", written);

  return written;
}

static RemixCount
remix_squaretone_process (RemixEnv * env, RemixBase * base, RemixCount count,
                          RemixStream * input, RemixStream * output)
{
  RemixSquareTone * squaretone = (RemixSquareTone *)base;
  return remix_stream_chunkfuncify (env, output, count,
                                    remix_squaretone_write_chunk, squaretone);
}

static RemixCount
remix_squaretone_length (RemixEnv * env, RemixBase * base)
{
  return REMIX_COUNT_INFINITE;
}

static RemixCount
remix_squaretone_seek (RemixEnv * env, RemixBase * base, RemixCount offset)
{
  RemixSquareTone * squaretone = (RemixSquareTone *)base;
  RemixCount wavelength;
  CDSet * s;
  RemixCount cycle_offset;
  RemixSquareToneChannel * sqch;

  wavelength = (RemixCount)(remix_get_samplerate (env) / squaretone->frequency);
  cycle_offset = offset % wavelength;

  for (s = squaretone->channels; s; s = s->next) {
    sqch = (RemixSquareToneChannel *)s->data.s_pointer;
    sqch->_cycle_offset = cycle_offset;
  }

  return offset;
}

static struct _RemixMethods _remix_squaretone_methods = {
  remix_squaretone_clone,
  remix_squaretone_destroy,
  remix_squaretone_ready,
  remix_squaretone_prepare,
  remix_squaretone_process,
  remix_squaretone_length,
  remix_squaretone_seek,
  NULL, /* flush */
};

static RemixSquareTone *
remix_squaretone_optimise (RemixEnv * env, RemixSquareTone * squaretone)
{
  _remix_set_methods (env, squaretone, &_remix_squaretone_methods);
  return squaretone;
}
