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
 * RemixSound: An instance of an base within an RemixLayer sequence.
 *
 * Conrad Parker <Conrad.Parker@CSIRO.AU>, August 2001
 *
 * Description
 * -----------
 *
 * A sound is contained within a layer. Each sound is a unique entity, but
 * many sounds may have the same source base.
 *
 * Invariants
 * ----------
 *
 * A sound must be contained in a layer.
 */

#include <string.h>

#define __REMIX__
#include "remix.h"

/* Optimisation dependencies: none */
static RemixSound * remix_sound_optimise (RemixEnv * env, RemixSound * sound);

/*
 * remix_sound_replace_mixstreams (env, sound)
 *
 * Replaces mix- (and envelope-) streams of 'sound' with new ones of
 * the env's mixlength.
 */
static void
remix_sound_replace_mixstreams (RemixEnv * env, RemixSound * sound)
{
  RemixCount mixlength = _remix_base_get_mixlength (env, sound);

  if (sound->_rate_envstream != RemixNone)
    remix_destroy (env, (RemixBase *)sound->_rate_envstream);
  if (sound->_gain_envstream != RemixNone)
    remix_destroy (env, (RemixBase *)sound->_gain_envstream);
  if (sound->_blend_envstream != RemixNone)
    remix_destroy (env, (RemixBase *)sound->_blend_envstream);

  sound->_rate_envstream =
    remix_stream_new_contiguous (env, mixlength);
  sound->_gain_envstream =
    remix_stream_new_contiguous (env, mixlength);
  sound->_blend_envstream =
    remix_stream_new_contiguous (env, mixlength);
}

static RemixBase *
remix_sound_init (RemixEnv * env, RemixBase * base)
{
  RemixSound * sound = (RemixSound *)base;
  sound->rate_envelope = sound->gain_envelope = sound->blend_envelope =
    RemixNone;
  sound->cutin = sound->cutlength = 0;
  sound->_rate_envstream = sound->_gain_envstream = sound->_blend_envstream =
    RemixNone;
  remix_sound_replace_mixstreams (env, sound);
  remix_sound_optimise (env, sound);
  return (RemixBase *)sound;
}

static RemixSound *
_remix_sound_new (RemixEnv * env)
{
  return (RemixSound *)
    remix_base_new_subclass (env, sizeof (struct _RemixSound));
}

RemixBase *
remix_sound_clone_invalid (RemixEnv * env, RemixBase * base)
{
  RemixSound * sound = (RemixSound *)base;
  RemixSound * new_sound = _remix_sound_new (env);

  memcpy (new_sound, sound, sizeof (struct _RemixSound));
  new_sound->layer = RemixNone;

  return (RemixBase *)new_sound;
}

RemixBase *
remix_sound_clone_with_layer (RemixEnv * env, RemixBase * base, RemixLayer * new_layer)
{
  RemixSound * sound = (RemixSound *)base;
  RemixSound * new_sound = _remix_sound_new (env);

  memcpy (new_sound, sound, sizeof (struct _RemixSound));
  new_sound->layer = new_layer;
  _remix_layer_add_sound (env, new_layer, new_sound, new_sound->start_time);

  return (RemixBase *)new_sound;
}

/* XXX: This breaks sound invariant */
RemixSound *
_remix_sound_remove (RemixEnv * env, RemixSound * sound)
{
  if (sound->layer == RemixNone) return RemixNone;
  return _remix_layer_remove_sound (env, sound->layer, sound);
}


static int
remix_sound_destroy (RemixEnv * env, RemixBase * base)
{
  RemixSound * sound = (RemixSound *)base;

  _remix_sound_remove (env, sound);

  if (sound->rate_envelope)
    remix_destroy (env, sound->rate_envelope);
  if (sound->gain_envelope)
    remix_destroy (env, sound->gain_envelope);
  if (sound->blend_envelope)
    remix_destroy (env, sound->blend_envelope);
  if (sound->_rate_envstream)
    remix_destroy (env, (RemixBase *)sound->_rate_envstream);
  if (sound->_gain_envstream)
    remix_destroy (env, (RemixBase *)sound->_gain_envstream);
  if (sound->_blend_envstream)
    remix_destroy (env, (RemixBase *)sound->_blend_envstream);
  remix_free (sound);

  return 0;
}

static int
remix_sound_ready (RemixEnv * env, RemixBase * base)
{
  return 0;
}

static RemixBase *
remix_sound_prepare (RemixEnv * env, RemixBase * base)
{
  RemixSound * sound = (RemixSound *)base;
  remix_sound_replace_mixstreams (env, sound);
  return base;
}

RemixBase *
remix_sound_set_source (RemixEnv * env, RemixSound * sound, RemixBase * source)
{
  RemixBase * old = sound->source;
  sound->source = source;
  return old;
}

RemixBase *
remix_sound_get_source (RemixEnv * env, RemixSound * sound)
{
  return sound->source;
}

RemixSound *
remix_sound_new (RemixEnv * env, RemixBase * source, RemixLayer * layer,
		 RemixTime start_time, RemixTime duration)
{
  RemixSound * sound = _remix_sound_new (env);

  sound->layer = layer;
  sound->start_time = start_time;
  sound->duration = duration;
  _remix_layer_add_sound (env, layer, sound, start_time);
  sound->source = source;
  remix_sound_init (env, (RemixBase *)sound);
  return sound;
}

RemixLayer *
remix_sound_get_layer (RemixEnv * env, RemixSound * sound)
{
  return sound->layer;
}

RemixTrack *
remix_sound_get_track (RemixEnv * env, RemixSound * sound)
{
  if (sound->layer == RemixNone) return RemixNone;
  return remix_layer_get_track (env, sound->layer);
}

RemixDeck *
remix_sound_get_deck (RemixEnv * env, RemixSound * sound)
{
  if (sound->layer == RemixNone) return RemixNone;
  return remix_layer_get_deck (env, sound->layer);
}

RemixTime
remix_sound_move (RemixEnv * env, RemixSound * sound, RemixTime start_time)
{
  RemixTime old = sound->start_time;
  RemixLayer * layer = sound->layer;
  if (_remix_sound_remove (env, sound) != RemixNone) {
    _remix_layer_add_sound (env, layer, sound, start_time);
    return old;
  } else {
    return _remix_time_invalid (sound->layer->timetype);
  } 
}

RemixSound *
remix_sound_get_prev (RemixEnv * env, RemixSound * sound)
{
  if (sound->layer)
    return _remix_layer_get_sound_prev (env, sound->layer, sound);
  else
    return RemixNone;
}

RemixSound *
remix_sound_get_next (RemixEnv * env, RemixSound * sound)
{
  if (sound->layer)
    return _remix_layer_get_sound_next (env, sound->layer, sound);
  else
    return RemixNone;
}

int
remix_sound_later (RemixEnv * env, RemixSound * s1, RemixSound * s2)
{
  RemixLayer * layer = remix_sound_get_layer (env, s1);
  return _remix_time_gt (layer->timetype, s1->start_time, s2->start_time);
}

RemixTime
remix_sound_set_start_time (RemixEnv * env, RemixSound * sound, RemixTime start_time)
{
  RemixTime old = sound->start_time;
  RemixLayer * layer = sound->layer;
  _remix_layer_remove_sound (env, layer, sound);
  _remix_layer_add_sound (env, layer, sound, start_time);
  return old;
}

RemixTime
remix_sound_get_start_time (RemixEnv * env, RemixSound * sound)
{
  return sound->start_time;
}

RemixTime
remix_sound_set_duration (RemixEnv * env, RemixSound * sound, RemixTime duration)
{
  RemixTime old = sound->duration;
  sound->duration = duration;
  return old;
}

RemixTime
remix_sound_get_duration (RemixEnv * env, RemixSound * sound)
{
  return sound->duration;
}

RemixBase *
remix_sound_set_rate_envelope (RemixEnv * env, RemixSound * sound,
			    RemixBase * rate_envelope)
{
  RemixBase * old = sound->rate_envelope;
  sound->rate_envelope = rate_envelope;
  return old;
}

RemixBase *
remix_sound_get_rate_envelope (RemixEnv * env, RemixSound * sound)
{
  if (sound == RemixNone) {
    remix_set_error (env, REMIX_ERROR_INVALID);
    return RemixNone;
  }
  return sound->rate_envelope;
}

RemixBase *
remix_sound_set_gain_envelope (RemixEnv * env, RemixSound * sound,
			    RemixBase * gain_envelope)
{
  RemixBase * old;

  if (sound == RemixNone) {
    remix_set_error (env, REMIX_ERROR_INVALID);
    return RemixNone;
  }
  old = sound->gain_envelope;
  sound->gain_envelope = gain_envelope;

  return old;
}

RemixBase *
remix_sound_get_gain_envelope (RemixEnv * env, RemixSound * sound)
{
  if (sound == RemixNone) {
    remix_set_error (env, REMIX_ERROR_INVALID);
    return RemixNone;
  }
  return sound->gain_envelope;
}

RemixBase *
remix_sound_set_blend_envelope (RemixEnv * env, RemixSound * sound,
			     RemixBase * blend_envelope)
{
  RemixBase * old = sound->blend_envelope;
  sound->blend_envelope = blend_envelope;
  return old;
}

RemixBase *
remix_sound_get_blend_envelope (RemixEnv * env, RemixSound * sound)
{
  if (sound == RemixNone) {
    remix_set_error (env, REMIX_ERROR_INVALID);
    return RemixNone;
  }
  return sound->blend_envelope;
}

static RemixCount
_remix_sound_fade (RemixEnv * env, RemixSound * sound, RemixCount count,
		  RemixStream * input, RemixStream * output)
{
  RemixCount output_offset, n;

  output_offset = remix_tell (env, (RemixBase *)output);

  /* XXX: this wanted to use 'block' not 'count': have we bounds
   * checked the sound ?? */

  remix_seek (env, (RemixBase *)sound->_blend_envstream, 0, SEEK_SET);
  n = remix_process (env, sound->blend_envelope, count, RemixNone,
		  sound->_blend_envstream);
  remix_stream_write (env, output, count, input);
  remix_seek (env, (RemixBase *)sound->_blend_envstream, 0, SEEK_SET);
  remix_seek (env, (RemixBase *)output, output_offset, SEEK_SET);
  n = remix_stream_fade (env, sound->_blend_envstream, output, count);

  return n;
}

/* Do rate conversion, handle offset etc.: get raw sound data */
static RemixCount
_remix_sound_get_raw (RemixEnv * env, RemixSound * sound, RemixCount offset,
		     RemixCount count, RemixStream * input, RemixStream * output)
{
  RemixCount block, n;

  remix_dprintf ("[_remix_sound_get_raw] (%p, +%ld, %p -> %p) @ %ld\n",
	      sound, count, input, output, offset);

  /* XXX: deal with rate conversion!! */

  if (sound->cutlength > 0) {
    if (offset > sound->cutlength) {
      remix_set_error (env, REMIX_ERROR_SILENCE);
      return -1;
    }
    block = MIN (count, sound->cutlength - offset);
  } else {
    block = count;
  }

  remix_dprintf ("[_remix_sound_get_raw] block +%ld (cutin: %ld, cutlength: %ld)\n",
	      block, sound->cutin, sound->cutlength);

  remix_seek (env, sound->source, sound->cutin + offset, SEEK_SET);
  n = remix_process (env, sound->source, block, input, output);

  if (n == -1) {
    remix_dprintf ("error getting source data: %s\n",
		remix_error_string(env, remix_last_error(env)));
  } else {

    if (block < count)
      n += remix_stream_write0 (env, output, count - block);
  }

  remix_dprintf ("[_remix_sound_get_raw] got %ld raw samples\n", n);

  return n;
}

static RemixCount
_remix_sound_apply_gain (RemixEnv * env, RemixSound * sound,
		      RemixCount offset, RemixCount count,
		      RemixStream * data, RemixCount data_offset)
{
  RemixCount n;

  remix_dprintf ("in _remix_sound_apply_gain (%p, +%ld)\n", sound, count);

  remix_seek (env, sound->gain_envelope, offset, SEEK_SET);
  remix_seek (env, (RemixBase *)sound->_gain_envstream, 0, SEEK_SET);
  n = remix_process (env, sound->gain_envelope, count,
		  RemixNone, sound->_gain_envstream);
  remix_dprintf ("Got %ld values from gain_envelope %p onto stream %p\n",
	  n, sound->gain_envelope, sound->_gain_envstream);

  remix_seek (env, (RemixBase *)data, data_offset, SEEK_SET);
  remix_seek (env, (RemixBase *)sound->_gain_envstream, 0, SEEK_SET);
  n = remix_stream_mult (env, sound->_gain_envstream, data, n);
  remix_dprintf ("Multiplied %ld values of gain\n", n);

  return n;
}

static RemixCount
_remix_sound_blend (RemixEnv * env, RemixSound * sound, RemixCount count,
		   RemixStream * input, RemixStream * output)
{
  RemixCount n;

  /* XXX: this wanted to use 'block' not 'count': have we bounds
   * checked the sound ?? */

  remix_seek (env, (RemixBase * )sound->_blend_envstream, 0, SEEK_SET);
  n = remix_process (env, sound->blend_envelope, count, RemixNone,
		    sound->_blend_envstream);
  remix_seek (env, (RemixBase *)sound->_blend_envstream, 0, SEEK_SET);
  n = remix_stream_blend (env, input, sound->_blend_envstream, output,
			 count);

  return n;
}

static RemixCount
remix_sound_process (RemixEnv * env, RemixBase * base, RemixCount count, RemixStream * input,
                  RemixStream * output)
{
  RemixSound * sound = (RemixSound *)base;
  RemixCount remaining = count, processed = 0, block, m, n;
  RemixCount offset, input_offset, output_offset;
  RemixCount mixlength = _remix_base_get_mixlength (env, sound);

  offset = remix_tell (env, (RemixBase *)sound);

  remix_dprintf ("PROCESS SOUND (%p [%p], +%ld, %p -> %p) @ %ld\n",
	  sound, sound ? sound->source : NULL, count, input, output, offset);

  /* if we're beyond the source's actual length, then
   * just fade the input to the output using the sound's blend
   * envelope */
  if (offset > remix_length (env, sound->source)) {
    remix_dprintf ("## offset %ld > length %ld\n", offset,
	    remix_length (env, sound->source));

    /* Fade input and copy to output */
    if (sound->blend_envelope == RemixNone) {
      remix_set_error (env, REMIX_ERROR_NOOP);
      return -1;
    } else {
      return _remix_sound_fade (env, sound, count, input, output);
    }
  }

  while (remaining > 0) {
    block = MIN (remaining, mixlength);

    input_offset = remix_tell (env, (RemixBase *)input);
    output_offset = remix_tell (env, (RemixBase *)output);

    /* Get raw sound data; handle rate conversion etc. */
    m = _remix_sound_get_raw (env, sound, offset, block, input, output);
    if (m == -1) {
      remix_dprintf ("error getting raw sound data\n");
      break;
    } else {
      n = m;
    }

    /* Apply gain envelope */
    if (sound->gain_envelope != RemixNone) {
      m = _remix_sound_apply_gain (env, sound, offset, n, output, output_offset);
      if (m == -1) {
	remix_dprintf ("error applying gain!\n");
      } else {
	n = m;
      }
    }
    
    /* Blend input back in */
    if (sound->blend_envelope != RemixNone) {
      remix_seek (env, (RemixBase *)input, input_offset, SEEK_SET);
      remix_seek (env, (RemixBase *)output, output_offset, SEEK_SET);
      n = _remix_sound_blend (env, sound, n, input, output);
    }
    
    offset += n;
    processed += n;
    remaining -= n;
  }

  remix_dprintf ("[remix_sound_process] processed %ld from sound %p\n",
	      processed, sound);

  return processed;
}

static RemixCount
remix_sound_length (RemixEnv * env, RemixBase * base)
{
  RemixSound * sound = (RemixSound *)base;
  RemixTime duration = remix_sound_get_duration (env, sound);
  RemixTime t = remix_time_convert (env, duration, sound->layer->timetype,
			      REMIX_TIME_SAMPLES);
  return t.samples;
}

static RemixCount
remix_sound_seek (RemixEnv * env, RemixBase * base, RemixCount offset)
{
  RemixSound * sound = (RemixSound *)base;
  if (sound->cutlength > 0 && offset > sound->cutlength) {
    offset = sound->cutlength;
  }
  remix_seek (env, sound->source, sound->cutin + offset, SEEK_SET);
  return offset;
}

static int
remix_sound_flush (RemixEnv * env, RemixBase * base)
{
  RemixSound * sound = (RemixSound *)base;
  return remix_flush (env, sound->source);
}

static struct _RemixMethods _remix_sound_methods = {
  remix_sound_clone_invalid, /* clone */
  remix_sound_destroy,       /* destroy */
  remix_sound_ready,         /* ready */
  remix_sound_prepare,       /* prepare */
  remix_sound_process,       /* process */
  remix_sound_length,        /* length */
  remix_sound_seek,          /* seek */
  remix_sound_flush,         /* flush */
};

static RemixSound *
remix_sound_optimise (RemixEnv * env, RemixSound * sound)
{
  _remix_set_methods (env, sound, &_remix_sound_methods);
  return sound;
}

