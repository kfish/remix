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
 * RemixTrack: An RemixLayer mixing abstraction.
 *
 * Conrad Parker <Conrad.Parker@CSIRO.AU>, August 2001
 *
 * Description
 * -----------
 *
 * A track is contained within a deck. A track contains a number of
 * layers which are mixed in series.
 *
 * Invariants
 * ----------
 *
 * A track must be contained within a deck.
 */

#define __REMIX__
#include "remix.h"

/* Optimisation dependencies: optimise on change of nr. layers */
static RemixTrack * remix_track_optimise (RemixEnv * env, RemixTrack * track);

static void
remix_track_replace_mixstreams (RemixEnv * env, RemixTrack * track)
{
  RemixCount mixlength = _remix_base_get_mixlength (env, track);

  if (track->_mixstream_a != RemixNone)
    remix_destroy (env, (RemixBase *)track->_mixstream_a);
  if (track->_mixstream_b != RemixNone)
    remix_destroy (env, (RemixBase *)track->_mixstream_b);

  track->_mixstream_a = remix_stream_new_contiguous (env, mixlength);
  track->_mixstream_b = remix_stream_new_contiguous (env, mixlength);
}

static RemixBase *
remix_track_init (RemixEnv * env, RemixBase * base)
{
  RemixTrack * track = (RemixTrack *)base;
  track->gain = 1.0;
  track->layers = cd_list_new (env);
  track->_mixstream_a = track->_mixstream_b = RemixNone;
  remix_track_replace_mixstreams (env, track);
  remix_track_optimise (env, track);
  return (RemixBase *)track;
}

static RemixTrack *
_remix_track_new (RemixEnv * env)
{
  return (RemixTrack *)
    remix_base_new_subclass (env, sizeof (struct _RemixTrack));
}

RemixBase *
remix_track_clone (RemixEnv * env, RemixBase * base)
{
  RemixTrack * track = (RemixTrack *)base;
  RemixTrack * new_track = _remix_track_new (env);

  new_track->gain = track->gain;
  new_track->layers = cd_list_clone (env, track->layers,
				     (CDCloneFunc)remix_layer_clone);
  remix_track_optimise (env, track);

  return (RemixBase *)new_track;
}

static int
remix_track_destroy (RemixEnv * env, RemixBase * base)
{
  RemixTrack * track = (RemixTrack *)base;
  remix_destroy_list (env, track->layers);
  remix_free (track);
  return 0;
}

static int
remix_track_ready (RemixEnv * env, RemixBase * base)
{
  return (remix_base_encompasses_mixlength (env, base) &&
	  remix_base_encompasses_channels (env, base));
}

static RemixBase *
remix_track_prepare (RemixEnv * env, RemixBase * base)
{
  RemixTrack * track = (RemixTrack *)base;
  remix_track_replace_mixstreams (env, track);
  return base;
}

RemixTrack *
remix_track_new (RemixEnv * env, RemixDeck * deck)
{
  RemixTrack * track;

  track = _remix_track_new (env);
  track->deck = deck;
  remix_track_init (env, (RemixBase *)track);
  _remix_deck_add_track (env, deck, track);

  return track;
}

static RemixCount
remix_track_length (RemixEnv * env, RemixBase * base)
{
  RemixTrack * track = (RemixTrack *)base;
  RemixCount length, maxlength = 0;
  CDList * l;
  RemixLayer * layer;

  for (l = track->layers; l; l = l->next) {
    layer = (RemixLayer *)l->data.s_pointer;
    length = remix_length (env, (RemixBase *)layer);
    remix_dprintf ("[remix_track_length] found layer %p length %ld\n",
		layer, length);
    maxlength = MAX (maxlength, length);
  }

  return maxlength;
}

RemixPCM
remix_track_set_gain (RemixEnv * env, RemixTrack * track, RemixPCM gain)
{
  RemixPCM old = track->gain;
  track->gain = gain;
  return old;
}

RemixPCM
remix_track_get_gain (RemixEnv * env, RemixTrack * track)
{
  return track->gain;
}

void
remix_remove_track (RemixEnv * env, RemixTrack * track)
{
  RemixDeck * deck = track->deck;

  if (deck)
    _remix_deck_remove_track (env, deck, track);
}

RemixDeck *
remix_track_get_deck (RemixEnv * env, RemixTrack * track)
{
  return track->deck;
}

/*
 * _remix_track_add_layer_above (env, track, layer, above)
 *
 * Adds 'layer' above 'above'. If above is RemixNone, adds 'layer' on top
 * of 'track'.
 */
RemixLayer *
_remix_track_add_layer_above (RemixEnv * env, RemixTrack * track,
                              RemixLayer * layer, RemixLayer * above)
{
  layer->track = track;
  if (above == RemixNone)
    above = (RemixLayer *)
      (cd_list_last (env, track->layers, CD_TYPE_POINTER)).s_pointer;
  track->layers = cd_list_add_after (env, track->layers, CD_TYPE_POINTER,
				     CD_POINTER(layer), CD_POINTER(above));
  remix_track_optimise (env, track);
  return layer;
}

RemixLayer *
_remix_track_remove_layer (RemixEnv * env, RemixTrack * track,
                           RemixLayer * layer)
{
  track->layers = cd_list_remove (env, track->layers, CD_TYPE_POINTER,
				  CD_POINTER(layer));
  remix_track_optimise (env, track);
  return layer;
}

/*
 * gets layer above 'above'. If above is RemixNone, returns topmost layer
 */
RemixLayer *
_remix_track_get_layer_above (RemixEnv * env, RemixTrack * track,
                              RemixLayer * above)
{
  CDList * above_item;
  RemixLayer * layer;

  if (track->layers == NULL) return RemixNone;

  if (above == RemixNone) {
    layer = (RemixLayer *)
      (cd_list_last (env, track->layers, CD_TYPE_POINTER)).s_pointer;
  } else {
    above_item = cd_list_find (env, track->layers, CD_TYPE_POINTER,
			       CD_POINTER(above));
    if (above_item == NULL) return RemixNone;
    above_item = above_item->next;
    if (above_item == NULL) return RemixNone;
    layer = (RemixLayer *) above_item->data.s_pointer;
  }

  return layer;
}

/*
 * gets layer below 'below'. If below is RemixNone, returns lowest layer
 */
RemixLayer *
_remix_track_get_layer_below (RemixEnv * env, RemixTrack * track,
                              RemixLayer * below)
{
  CDList * below_item;
  RemixLayer * layer;

  if (track->layers == NULL) return RemixNone;

  if (below == RemixNone) {
    layer = (RemixLayer *) track->layers->data.s_pointer;
  } else {
    below_item = cd_list_find (env, track->layers, CD_TYPE_POINTER,
			       CD_POINTER(below));
    if (below_item == NULL) return RemixNone;
    below_item = below_item->prev;
    if (below_item == NULL) return RemixNone;
    layer = (RemixLayer *) below_item->data.s_pointer;
  }

  return layer;
}


static RemixCount
remix_track_process (RemixEnv * env, RemixBase * base, RemixCount count,
                     RemixStream * input, RemixStream * output)
{
  RemixTrack * track = (RemixTrack *)base;
  CDList * l;
  RemixBase * layer;
  RemixStream * si, * so, * swap_stream;
  RemixCount remaining = count, processed = 0, n = 0;
  RemixCount mixlength = _remix_base_get_mixlength (env, track);

  remix_dprintf ("PROCESS TRACK (%p, +%ld, %p -> %p) @ %ld\n",
		track, count, input, output, remix_tell (env, base));

  if (track->layers == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOOP);
    return 0;
  }

  while (remaining > 0) {
    l = track->layers;
    si = input;
    so = track->_mixstream_a;
    n = MIN (remaining, mixlength);

    while (l) {
      layer = (RemixBase *)l->data.s_pointer;

      if (si == input) {
	swap_stream = track->_mixstream_b;
      } else {
	swap_stream = si;
	remix_seek (env, (RemixBase *)si, 0, SEEK_SET);
      }
      
      if (l->next == RemixNone) {
	so = output;
      } else {
	remix_seek (env, (RemixBase *)so, 0, SEEK_SET);
      }
      
      n = remix_process (env, layer, n, si, so);
      
      l = l->next;

      /* swap si, st using swap_stream as evaluated above */
      si = so; so = swap_stream;
    }
    remaining -= n;
    processed += n;
  }

  remix_dprintf ("[remix_track_process] processed %ld\n", processed);

  return processed;
}

static RemixCount
remix_track_twolayer_process (RemixEnv * env, RemixBase * base,
                              RemixCount count, RemixStream * input,
                              RemixStream * output)
{
  RemixTrack * track = (RemixTrack *)base;
  CDList * l;
  RemixBase * layer1, * layer2;
  RemixCount remaining = count, processed = 0, n = 0;
  RemixStream * mix = track->_mixstream_a;
  RemixCount current_offset = remix_tell (env, base);
  RemixCount mixlength = _remix_base_get_mixlength (env, track);

  remix_dprintf ("PROCESS TRACK [twolayer] (%p, +%ld, %p -> %p) @ %ld\n",
	      track, count, input, output, current_offset);

  l = track->layers;
  layer1 = (RemixBase *)l->data.s_pointer;
  remix_seek (env, (RemixBase *)layer1, current_offset, SEEK_SET);
  
  l = l->next;
  layer2 = (RemixBase *)l->data.s_pointer;
  remix_seek (env, (RemixBase *)layer2, current_offset, SEEK_SET);

  while (remaining > 0) {
    n = MIN (remaining, mixlength);

    remix_seek (env, (RemixBase *)mix, 0, SEEK_SET);
    n = remix_process (env, layer1, n, input, mix);

    remix_seek (env, (RemixBase *)mix, 0, SEEK_SET);
    n = remix_process (env, layer2, n, mix, output);

    remaining -= n;
    processed += n;
  }

  remix_dprintf ("*** PRE-SEEK: track @ %ld\n", remix_tell (env, base));

  /*remix_seek (env, base, current_offset + processed, SEEK_SET);*/

  remix_dprintf ("*** POST-SEEK: track @ %ld\n", remix_tell (env, base));

  remix_dprintf ("[remix_track_twolayer_process] processed %ld\n", processed);

  return processed;
}

static RemixCount
remix_track_onelayer_process (RemixEnv * env, RemixBase * base,
                              RemixCount count, RemixStream * input,
                              RemixStream * output)
{
  RemixTrack * track = (RemixTrack *)base;
  CDList * l = track->layers;
  RemixBase * layer = (RemixBase *)l->data.s_pointer;
  RemixCount n;

  remix_dprintf ("PROCESS TRACK [onelayer] (%p, +%ld, %p -> %p) @ %ld\n",
	      track, count, input, output, remix_tell (env, base));

  n = remix_process (env, layer, count, input, output);

  remix_dprintf ("[remix_track_onelayer_process] processed %ld\n", n);

  return n;
}

static RemixCount
remix_track_seek (RemixEnv * env, RemixBase * base, RemixCount offset)
{
  RemixTrack * track = (RemixTrack *)base;
  CDList * l;
  RemixLayer * layer;

  for (l = track->layers; l; l = l->next) {
    layer = (RemixLayer *)l->data.s_pointer;
    remix_seek (env, (RemixBase *)layer, offset, SEEK_SET);
  }

  return offset;
}

static int
remix_track_flush (RemixEnv * env, RemixBase * base)
{
  RemixTrack * track = (RemixTrack *)base;
  CDList * l;
  RemixLayer * layer;

  for (l = track->layers; l; l = l->next) {
    layer = (RemixLayer *)l->data.s_pointer;
    remix_flush (env, (RemixBase *)layer);
  }

  return 0;
}

static struct _RemixMethods _remix_track_empty_methods = {
  remix_track_clone,   /* clone */
  remix_track_destroy, /* destroy */
  remix_track_ready,   /* ready */
  remix_track_prepare, /* prepare */
  remix_null_process,  /* process */
  remix_null_length,   /* length */
  remix_null_seek,     /* seek */
  NULL,             /* flush */
};

static struct _RemixMethods _remix_track_methods = {
  remix_track_clone,   /* clone */
  remix_track_destroy, /* destroy */
  remix_track_ready,   /* ready */
  remix_track_prepare, /* prepare */
  remix_track_process, /* process */
  remix_track_length,  /* length */
  remix_track_seek,    /* seek */
  remix_track_flush,            /* flush */
};

static struct _RemixMethods _remix_track_onelayer_methods = {
  remix_track_clone,            /* clone */
  remix_track_destroy,          /* destroy */
  remix_track_ready,            /* ready */
  remix_track_prepare,          /* prepare */
  remix_track_onelayer_process, /* process */
  remix_track_length,           /* length */
  remix_track_seek,             /* seek */
  remix_track_flush,            /* flush */
};

static struct _RemixMethods _remix_track_twolayer_methods = {
  remix_track_clone,            /* clone */
  remix_track_destroy,          /* destroy */
  remix_track_ready,            /* ready */
  remix_track_prepare,          /* prepare */
  remix_track_twolayer_process, /* process */
  remix_track_length,           /* length */
  remix_track_seek,             /* seek */
  remix_track_flush,            /* flush */
};

static RemixTrack *
remix_track_optimise (RemixEnv * env, RemixTrack * track)
{
  RemixCount nr_layers = cd_list_length (env, track->layers);

  switch (nr_layers) {
  case 0:
    _remix_set_methods (env, track, &_remix_track_empty_methods); break;
  case 1:
    _remix_set_methods (env, track, &_remix_track_onelayer_methods); break;
  case 2:
    _remix_set_methods (env, track, &_remix_track_twolayer_methods); break;
  default:
      _remix_set_methods (env, track, &_remix_track_methods); break;
  }

  return track;
}
