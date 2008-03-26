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
 * RemixLayer: A sound sequence abstraction.
 *
 * Conrad Parker <conrad@metadecks.org>, August 2001
 *
 * Description
 * -----------
 *
 * A layer is contained within a track. It consists of a sequence of
 * sounds. A layer has a time type (RemixTimeType) with which its sounds
 * are indexed.
 * XXX: Overlapping sounds in a layer should be mixed together.
 *
 * Invariants
 * ----------
 *
 * A layer must be contained in a track.
 *
 */

#define __REMIX__
#include "remix.h"

/* Optimisation dependencies: none */
static RemixLayer * remix_layer_optimise (RemixEnv * env, RemixLayer * layer);

/* Coherency dependencies: ensure coherency on addition+removal of sounds */
static RemixLayer * remix_layer_ensure_coherency (RemixEnv * env, RemixLayer * layer);

void
remix_layer_debug (RemixEnv * env, RemixLayer * layer)
{
#ifdef DEBUG
  CDList * l;
  RemixSound * s;

  remix_dprintf ("Layer (0x%p): ", layer);
  if (layer == RemixNone) return;

  for (l = layer->sounds; l; l = l->next) {
    s = (RemixSound *)l->data.s_pointer;
    /* XXX: assumes samples */
    remix_dprintf ("[0x%p: %ld, +%ld] ", s, s->start_time.samples,
		s->duration.samples);
  }
  remix_dprintf ("\n");
#endif
}

static RemixBase *
remix_layer_init (RemixEnv * env, RemixBase * base)
{
  RemixLayer * layer = (RemixLayer *)base;
  layer->timetype = REMIX_TIME_SAMPLES;
  layer->sounds = cd_list_new (env);
  /*  layer->_current_time = _remix_time_zero (layer->timetype);*/
  layer->_current_sound_item = RemixNone;
  layer->_current_tempo = remix_get_tempo (env);
  layer->_current_offset = 0;
  remix_layer_optimise (env, layer);
  return (RemixBase *)layer;
}

static RemixBase *
_remix_layer_new (RemixEnv * env)
{
  return (RemixBase *)
    remix_base_new_subclass (env, sizeof (struct _RemixLayer));
}

RemixBase *
remix_layer_clone (RemixEnv * env, RemixBase * base)
{
  RemixLayer * layer = (RemixLayer *)base;
  RemixLayer * new_layer = (RemixLayer *)_remix_layer_new (env);
  RemixCount offset = remix_tell (env, base);

  new_layer->timetype = layer->timetype;
  new_layer->sounds =
    cd_list_clone_with (env, layer->sounds,
			(CDCloneWithFunc)remix_sound_clone_with_layer,
			new_layer);
  remix_seek (env, (RemixBase *)new_layer, offset, SEEK_SET);

  new_layer->track = layer->track;
  _remix_track_add_layer_above (env, layer->track, new_layer, layer);

  return (RemixBase *)new_layer;
}

static int
remix_layer_destroy (RemixEnv * env, RemixBase * base)
{
  RemixLayer * layer = (RemixLayer *)base;
  if (layer->track)
    _remix_track_remove_layer (env, layer->track, layer);
  remix_destroy_list (env, layer->sounds);
  remix_free (layer);
  return 0;
}

RemixLayer *
remix_layer_new_ontop (RemixEnv * env, RemixTrack * track,
		       RemixTimeType timetype)
{
  RemixLayer * layer = (RemixLayer *)_remix_layer_new (env);

  layer->track = track;
  _remix_track_add_layer_above (env, track, layer, RemixNone);
  remix_layer_init (env, (RemixBase *)layer);
  layer->timetype = timetype;

  return layer;
}

RemixLayer *
remix_layer_new_above (RemixEnv * env, RemixLayer * above,
		       RemixTimeType timetype)
{
  RemixLayer * layer;
  if (above == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return RemixNone;
  }

  layer = (RemixLayer *)_remix_layer_new (env);
  layer->track = above->track;
  _remix_track_add_layer_above (env, above->track, layer, above);
  remix_layer_init (env, (RemixBase *)layer);
  layer->timetype = timetype;

  return layer;
}

RemixLayer *
remix_layer_move_ontop (RemixEnv * env, RemixLayer * layer, RemixTrack * track)
{
  if (layer->track)
    _remix_track_remove_layer (env, layer->track, layer);

  if (track)
    _remix_track_add_layer_above (env, track, layer, RemixNone);

  return layer;
}

RemixLayer *
remix_layer_move_above (RemixEnv * env, RemixLayer * layer, RemixLayer * above)
{
  if (layer->track)
    _remix_track_remove_layer (env, layer->track, layer);

  if (above->track)
    _remix_track_add_layer_above (env, above->track, layer, above);

  return layer;
}

RemixLayer *
remix_layer_raise (RemixEnv * env, RemixLayer * layer)
{
  RemixLayer * above;

  if (layer->track) {
    above = _remix_track_get_layer_above (env, layer->track, layer);
    remix_layer_move_above (env, layer, above);
  }

  return layer;
}

RemixLayer *
remix_layer_lower (RemixEnv * env, RemixLayer * layer)
{
  RemixLayer * below;

  if (layer->track) {
    below = _remix_track_get_layer_below (env, layer->track, layer);
    remix_layer_move_above (env, below, layer);
  }

  return layer;
}

RemixLayer *
_remix_remove_layer (RemixEnv * env, RemixLayer * layer)
{
  if (layer->track)
    _remix_track_remove_layer (env, layer->track, layer);

  layer->track = RemixNone;
  return layer;
}

RemixTrack *
remix_layer_get_track (RemixEnv * env, RemixLayer * layer)
{
  return layer->track;
}

RemixDeck *
remix_layer_get_deck (RemixEnv * env, RemixLayer * layer)
{
  RemixTrack * track = layer->track;
  if (track == RemixNone) return RemixNone;
  return remix_track_get_deck (env, track);
}

RemixTimeType
remix_layer_set_timetype (RemixEnv * env, RemixLayer * layer, RemixTimeType new_type)
{
  RemixTimeType old_type = layer->timetype;
  CDList * l;
  RemixDeck * deck;
  RemixSound * sound;

  if (old_type == new_type) return old_type;

  deck = remix_layer_get_deck (env, layer);

  for (l = layer->sounds; l; l = l->next) {
    sound = (RemixSound *)l->data.s_pointer;
    sound->start_time = remix_time_convert (env, sound->start_time, old_type,
					   new_type);
    sound->duration = remix_time_convert (env, sound->duration, old_type,
					 new_type);
  }

  layer->timetype = new_type;

  return old_type;
}

RemixTimeType
remix_layer_get_timetype (RemixEnv * env, RemixLayer * layer)
{
  return layer->timetype;
}

RemixSound *
_remix_layer_add_sound (RemixEnv * env, RemixLayer * layer, RemixSound * sound,
		     RemixTime start_time)
{
  sound->start_time = start_time;
  layer->sounds = cd_list_insert (env, layer->sounds, CD_TYPE_POINTER,
				  CD_POINTER(sound),
				  (CDCmpFunc)remix_sound_later);
  remix_layer_ensure_coherency (env, layer);
  return sound;
}

RemixSound *
_remix_layer_remove_sound (RemixEnv * env, RemixLayer * layer, RemixSound * sound)
{
  layer->sounds = cd_list_remove (env, layer->sounds, CD_TYPE_POINTER,
				  CD_POINTER(sound));
  remix_layer_ensure_coherency (env, layer);
  return sound;
}

/*
 * remix_layer_get_sound_item_before (layer, time)
 *
 * Finds the last sound with a start_time before 'time'.
 */
static CDList *
remix_layer_get_sound_item_before (RemixEnv * env, RemixLayer * layer, RemixTime time)
{
  RemixSound * s;
  CDList * l, * lp = RemixNone;

  for (l = layer->sounds; l; l = l->next) {
    s = (RemixSound *)l->data.s_pointer;
    if (_remix_time_gt (layer->timetype, s->start_time, time)) return lp;
    lp = l;
  }
  
  return lp;
}

/*
 * remix_layer_get_sound_item_at (layer, time)
 *
 * Finds the sound occurring at 'time'. If no sound is playing at 'time',
 * returns RemixNone.
 */
CDList *
remix_layer_get_sound_item_at (RemixEnv * env, RemixLayer * layer, RemixTime time)
{
  RemixTime t;
  CDList * l = remix_layer_get_sound_item_before (env, layer, time);
  RemixSound * s;

  if (l == RemixNone) return RemixNone;

  s = (RemixSound *)l->data.s_pointer;

  t = _remix_time_add (layer->timetype, s->start_time, s->duration);
  if (_remix_time_le (layer->timetype, t, time)) l = RemixNone;

  return l;
}

/*
 * remix_layer_get_sound_item_after (layer, time)
 *
 * Finds the first sound at or after 'time' and returns its list item.
 */
static CDList *
remix_layer_get_sound_item_after (RemixEnv * env, RemixLayer * layer, RemixTime time)
{
  RemixSound * s;
  CDList * l;

  for (l = layer->sounds; l; l = l->next) {
    s = (RemixSound *)l->data.s_pointer;
    if (_remix_time_ge (layer->timetype, s->start_time, time)) return l;
  }

  return RemixNone;
}

/*
 * remix_layer_get_sound_after (layer, time)
 *
 * Finds the first sound with a start_time at or after 'time'.
 */
RemixSound *
remix_layer_get_sound_after (RemixEnv * env, RemixLayer * layer, RemixTime time)
{
  CDList * l = remix_layer_get_sound_item_after (env, layer, time);

  if (l == RemixNone) return RemixNone;
  else return (RemixSound *)l->data.s_pointer;
}

RemixSound *
_remix_layer_get_sound_prev (RemixEnv * env, RemixLayer * layer, RemixSound * sound)
{
  CDList * sound_item;
  RemixSound * sn;

  if (layer->sounds == NULL) return RemixNone;

  if (sound == RemixNone) {
    sn = (RemixSound *) layer->sounds->data.s_pointer;
  } else {
    sound_item = cd_list_find (env, layer->sounds, CD_TYPE_POINTER,
			       CD_POINTER(sound));
    if (sound_item == NULL) return RemixNone;
    sound_item = sound_item->prev;
    if (sound_item == NULL) return RemixNone;
    sn = (RemixSound *) sound_item->data.s_pointer;
  }

  return sn;
}

RemixSound *
_remix_layer_get_sound_next (RemixEnv * env, RemixLayer * layer, RemixSound * sound)
{
  CDList * sound_item;
  RemixSound * sn;

  if (layer->sounds == NULL) return RemixNone;

  if (sound == NULL) {
    sn = (RemixSound *)
      (cd_list_last (env, layer->sounds, CD_TYPE_POINTER)).s_pointer;
  } else {
    sound_item = cd_list_find (env, layer->sounds, CD_TYPE_POINTER,
			       CD_POINTER(sound));
    if (sound_item == NULL) return RemixNone;
    sound_item = sound_item->next;
    if (sound_item == NULL) return RemixNone;
    sn = (RemixSound *) sound_item->data.s_pointer;
  }

  return sn;
}

RemixLayer *
remix_layer_below (RemixEnv * env, RemixLayer * layer)
{
  if (layer->track)
    return _remix_track_get_layer_below (env, layer->track, layer);
  else
    return RemixNone;
}

RemixLayer *
remix_layer_above (RemixEnv * env, RemixLayer * layer)
{
  if (layer->track)
    return _remix_track_get_layer_above (env, layer->track, layer);
  else
    return RemixNone;
}

static RemixCount
remix_layer_length (RemixEnv * env, RemixBase * base)
{
  RemixLayer * layer = (RemixLayer *)base;
  RemixSound * sound = (RemixSound *)
    (cd_list_last (env, layer->sounds, CD_TYPE_POINTER)).s_pointer;
  RemixTime end, t;

  if (sound == RemixNone) {
    remix_dprintf ("[remix_layer_length] layer %p has no sounds\n", layer);
    return 0;
  }

  /* Convert sound's end time to offset and return that */
  end = _remix_time_add (layer->timetype, sound->start_time, sound->duration);
  t = remix_time_convert (env, end, layer->timetype, REMIX_TIME_SAMPLES);

  remix_dprintf ("[remix_layer_length] (%p) last sound ends at %d ticks == %ld samples\n",
                 layer, end.beat24s, t.samples);

  return t.samples;
}

static RemixCount
remix_layer_seek (RemixEnv * env, RemixBase * base, RemixCount offset)
{
  RemixLayer * layer = (RemixLayer *)base;
  RemixTime current_time;

  /* Evaluate this offset as a time value using the current samplerate and
   * tempo */
  current_time = remix_time_convert (env, (RemixTime)offset, REMIX_TIME_SAMPLES,
				    layer->timetype);

  /* Cache the current sound item */
  layer->_current_sound_item =
    remix_layer_get_sound_item_at (env, layer, current_time);

  if (layer->_current_sound_item == RemixNone)
    layer->_current_sound_item =
      remix_layer_get_sound_item_after (env, layer, current_time);

  layer->_current_offset = offset;

  return offset;
}

static RemixCount
remix_layer_process_sound (RemixEnv * env, RemixLayer * layer,
			  RemixCount current_offset, RemixSound * sound,
			  RemixCount sound_offset, RemixCount sound_length,
			  RemixCount count,
			  RemixStream * input, RemixStream * output)
{
  RemixCount remaining = count, processed = 0, n;

  /* If the next sound starts after the current offset, fill up to it */
  if (sound_offset > current_offset) {
    n = MIN (remaining, sound_offset - current_offset);

    remix_dprintf ("[remix_layer_process_sound] %p is after offset, filling %ld\n",
		sound, n);

    if (output != RemixNone)
      n = remix_stream_write (env, output, n, input);
    current_offset += n;
    processed += n;
    remaining -= n;
  }

  /* If that didn't fill the output, process part of the next sound */
  if (processed < count) {
    n = MIN (remaining, sound_offset + sound_length - current_offset);
    /* XXX: fix following line to use SEEK_CUR ??? */
    remix_seek (env, (RemixBase *)sound, current_offset - sound_offset,
	       SEEK_SET);
    n = remix_process (env, (RemixBase *)sound, n, input, output);
    processed += n;
    remaining -= n;
  }

  return processed;
}

static RemixCount
remix_layer_process (RemixEnv * env, RemixBase * base, RemixCount count,
		    RemixStream * input, RemixStream * output)
{
  RemixLayer * layer = (RemixLayer *)base;
  RemixTempo tempo = remix_get_tempo (env);
  RemixCount processed = 0, remaining = count, n;
  RemixCount sound_offset, sound_length, next_offset;
  RemixCount current_offset = remix_tell (env, (RemixBase *)layer);
  RemixSound * sound, * sn;
  RemixTime t;

  remix_dprintf ("PROCESS LAYER (%p, +%ld, %p -> %p) @ %ld\n",
	      layer, count, input, output, current_offset);


  if (layer->timetype == REMIX_TIME_BEAT24S && layer->_current_tempo != tempo) {
#if 0
    RemixCount new_offset;

#if 0
    if (layer->_current_sound_item == RemixNone) {
      RemixSamplerate samplerate = remix_get_samplerate (env);
      int beat24s;

      beat24s = ((int)((float)remix_tell (env, (RemixBase *)layer) *
		       layer->_current_tempo * 24.0 / (samplerate * 60.0)));
      new_offset = (RemixCount)(beat24s * samplerate * 60.0 / (tempo * 24.0));
    } else {
      sound = (RemixSound *)layer->_current_sound_item->data.s_pointer;
      t = remix_time_convert (env, sound->start_time, layer->timetype,
			     REMIX_TIME_SAMPLES);
      
      new_offset = t.samples + remix_tell (env, (RemixBase *)sound);
    }
#else
    new_offset = layer->_current_offset * tempo / layer->_current_tempo;
#endif
  
    remix_layer_seek (env, (RemixBase *)layer, new_offset);
    layer->_current_tempo = tempo;
#else
    if (layer->_current_sound_item != RemixNone)
      remix_layer_ensure_coherency (env, layer);
#endif
  }

  while (remaining > 0) {

    if (layer->_current_sound_item == RemixNone) {
      /* No more sounds */
      remix_dprintf ("[remix_layer_process] ## no more sounds!\n");

      n = (output == RemixNone) ? remaining :
	remix_stream_write (env, output, remaining, input);
      current_offset += n;
      processed += n;
      remaining -= n;
      break;
    }

    sound = (RemixSound *)layer->_current_sound_item->data.s_pointer;
    t = remix_time_convert (env, sound->start_time, layer->timetype,
			   REMIX_TIME_SAMPLES);
    sound_offset = t.samples;

    t = remix_time_convert (env, sound->duration, layer->timetype,
			   REMIX_TIME_SAMPLES);
    sound_length = t.samples;

    if (layer->_current_sound_item->next) {
      sn = (RemixSound *)(layer->_current_sound_item->next->data.s_pointer);
      t = remix_time_convert (env, sn->start_time, layer->timetype,
			     REMIX_TIME_SAMPLES);
      next_offset = t.samples;
      if (next_offset < sound_offset + sound_length)
        sound_length = next_offset - sound_offset;
    }

    /* *** We now have the next sound and its valid length *** */
    remix_dprintf ("[remix_layer_process] to process sound %p, [%ld, +%ld]\n",
		  sound, sound_offset, sound_length);

    n = remix_layer_process_sound (env, layer, current_offset,
				  sound, sound_offset, sound_length,
				  remaining, input, output);
    current_offset += n;
    processed += n;
    remaining -= n;

    if (current_offset >= sound_offset + sound_length)
      layer->_current_sound_item = layer->_current_sound_item->next;
  }

  remix_dprintf ("[remix_layer_process] processed %ld\n", processed);

  if (processed == 0) {
    remix_set_error (env, REMIX_ERROR_NOOP);
    return -1;
  }

  layer->_current_offset = current_offset;

  return processed;
}

static int
remix_layer_flush (RemixEnv * env, RemixBase * base)
{
  RemixLayer * layer = (RemixLayer *)base;
  RemixBase * sound;

  if (layer->_current_sound_item == RemixNone) return 0;

  sound = (RemixBase *)layer->_current_sound_item->data.s_pointer;

  return remix_flush (env, sound);
}

static RemixLayer *
remix_layer_ensure_coherency (RemixEnv * env, RemixLayer * layer)
{
  RemixCount offset = remix_tell (env, (RemixBase *)layer);

  remix_layer_seek (env, (RemixBase *)layer, offset);
  layer->_current_tempo = remix_get_tempo (env);

  return layer;
}

static struct _RemixMethods _remix_layer_methods = {
  remix_layer_clone,   /* clone */
  remix_layer_destroy, /* destroy */
  NULL,             /* ready */
  NULL,             /* prepare */
  remix_layer_process, /* process */
  remix_layer_length,  /* length */
  remix_layer_seek,    /* seek */
  remix_layer_flush,   /* flush */
};

static RemixLayer *
remix_layer_optimise (RemixEnv * env, RemixLayer * layer)
{
  _remix_set_methods (env, layer, &_remix_layer_methods);
  return layer;
}
		  
