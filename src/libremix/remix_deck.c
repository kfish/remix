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
 * RemixDeck: A high level audio mixing abstraction.
 *
 * Conrad Parker <Conrad.Parker@CSIRO.AU>, August 2001
 *
 * Description
 * -----------
 *
 * A deck contains a number of tracks which are mixed in parallel.
 *
 * Invariants
 * ----------
 *
 * Decks are independent entities. The creation of a new deck does not
 * depend on the existence of any other bases. Any tracks (and, by
 * extension, layers and sounds) added to a deck become part of that
 * deck and are destroyed when the deck is destroyed.
 *
 */

#define __REMIX__
#include "remix.h"

/* Optimisation dependencies: optimise on change of nr. tracks */
static RemixDeck * remix_deck_optimise (RemixEnv * env, RemixDeck * deck);

static RemixDeck *
remix_deck_replace_mixstream (RemixEnv * env, RemixDeck * deck)
{
  RemixCount mixlength = _remix_base_get_mixlength (env, deck);

  if (deck->_mixstream != RemixNone)
    remix_destroy (env, (RemixBase *)deck->_mixstream);

  deck->_mixstream = remix_stream_new_contiguous (env, mixlength);

  return deck;
}

RemixBase *
remix_deck_init (RemixEnv * env, RemixBase * base)
{
  RemixDeck * deck = (RemixDeck *)base;
  deck->tracks = cd_list_new (env);
  deck->_mixstream = RemixNone;
  remix_deck_replace_mixstream (env, deck);
  remix_deck_optimise (env, deck);
  return (RemixBase *)deck;
}

RemixDeck *
remix_deck_new (RemixEnv * env)
{
  RemixDeck * deck =
    (RemixDeck *) remix_base_new_subclass (env, sizeof (struct _RemixDeck));
  return (RemixDeck *)remix_deck_init (env, (RemixBase *)deck);
}

RemixBase *
remix_deck_clone (RemixEnv * env, RemixBase * base)
{
  RemixDeck * deck = (RemixDeck *)base;
  RemixDeck * new_deck = remix_deck_new (env);
  new_deck->tracks = cd_list_clone (env, deck->tracks,
				    (CDCloneFunc)remix_track_clone);
  return (RemixBase *)new_deck;
}

static int
remix_deck_destroy (RemixEnv * env, RemixBase * base)
{
  RemixDeck * deck = (RemixDeck *)base;
  remix_destroy_list (env, deck->tracks);
  remix_destroy (env, (RemixBase *)deck->_mixstream);
  remix_free (deck);
  return 0;
}

static int
remix_deck_ready (RemixEnv * env, RemixBase * base)
{
  return (remix_base_encompasses_mixlength (env, base) &&
	  remix_base_encompasses_channels (env, base));
}

static RemixBase *
remix_deck_prepare (RemixEnv * env, RemixBase * base)
{
  RemixDeck * deck = (RemixDeck *)base;
  remix_deck_replace_mixstream (env, deck);
  return base;
}

RemixTrack *
_remix_deck_add_track (RemixEnv * env, RemixDeck * deck, RemixTrack * track)
{
  deck->tracks = cd_list_prepend (env, deck->tracks, CD_POINTER(track));
  remix_deck_optimise (env, deck);
  return track;
}

RemixTrack *
_remix_deck_remove_track (RemixEnv * env, RemixDeck * deck, RemixTrack * track)
{
  deck->tracks = cd_list_remove (env, deck->tracks, CD_TYPE_POINTER,
				 CD_POINTER(track));
  remix_deck_optimise (env, deck);
  return track;
}

int
remix_deck_nr_tracks (RemixEnv * env, RemixDeck * deck)
{
  return cd_list_length (env, deck->tracks);
}

static RemixCount
remix_deck_length (RemixEnv * env, RemixBase * base)
{
  RemixDeck * deck = (RemixDeck *)base;
  RemixCount length, maxlength = 0;
  CDList * l;
  RemixBase * track;
  
  for (l = deck->tracks; l; l = l->next) {
    track = (RemixBase *)l->data.s_pointer;
    length = remix_length (env, track);
    remix_dprintf ("[remix_deck_length] found track %p length %ld\n",
                   track, length);
    maxlength = MAX (maxlength, length);
  }

  return maxlength;
}

CDList *
remix_deck_get_tracks (RemixEnv * env, RemixDeck * deck)
{
  return cd_list_copy (env, deck->tracks);
}

#if 0
static RemixCount
remix_deck_process (RemixEnv * env, RemixBase base, RemixCount count, RemixStream input,
		 RemixStream output)
{
  RemixDeck deck = (RemixDeck)base;
  CDList tl, ml;
  RemixTrack track;
  RemixStream mixstream;
  RemixCount processed = 0, remaining = count, n, minn;
  RemixCount input_offset;

  remix_dprintf ("PROCESS DECK (%p, +%ld, %p -> %p) @ %ld\n",
	      deck, count, input, output, remix_tell (env, base));

  while (remaining > 0) {
    /* Get mixlength from each track */
    minn = deck->mixlength;
    input_offset = remix_tell (env, (RemixBase)input);
    for (tl = deck->tracks, ml = deck->mixstreams; tl && ml;
         tl = tl->next, ml = ml->next) {
      track = (RemixTrack)tl->data;
      mixstream = (RemixStream)ml->data;
      remix_seek (env, (RemixBase)input, input_offset);
      n = remix_process (env, (RemixBase)track, MIN (remaining, deck->mixlength),
		      input, mixstream);
      minn = MIN (minn, n);
    }


    if (output != RemixNone) {
      /* mix tracks together to the output */
      n = remix_streams_mix (env, deck->mixstreams, output, minn);
    } else {
      /* don't need to create output */
      n = minn;
    }

    processed += n;
    remaining -= n;
  }

  remix_dprintf ("[remix_deck_process] processed %ld\n", processed);

  return processed;
}
#endif

static RemixCount
remix_deck_process (RemixEnv * env, RemixBase * base, RemixCount count,
                    RemixStream * input, RemixStream * output)
{
  RemixDeck * deck = (RemixDeck *)base;
  CDList * l;
  RemixTrack * track;
  RemixCount remaining = count, processed = 0, n;
  RemixCount current_offset = remix_tell (env, base);
  RemixCount input_offset = remix_tell (env, (RemixBase *)input);
  RemixCount output_offset = remix_tell (env, (RemixBase *)output);
  RemixCount mixlength = _remix_base_get_mixlength (env, deck);
  RemixStream * mixstream = deck->_mixstream;

  remix_dprintf ("PROCESS DECK (%p, +%ld, %p -> %p) @ %ld\n",
                 deck, count, input, output, current_offset);

  while (remaining > 0) {

    l = deck->tracks;
    track = (RemixTrack *)l->data.s_pointer;

    n = MIN (remaining, mixlength);

    n = remix_process (env, (RemixBase *)track, n, input, output);
    remix_seek (env, (RemixBase *)output, output_offset, SEEK_SET);
    n = remix_stream_gain (env, output, n, track->gain);

    for (l = l->next; l; l = l->next) {
      track = (RemixTrack *)l->data.s_pointer;
      
      remix_seek (env, (RemixBase *)input, input_offset, SEEK_SET);
      remix_seek (env, (RemixBase *)mixstream, 0, SEEK_SET);
      n = remix_process (env, (RemixBase *)track, n, input, mixstream);
      
      remix_seek (env, (RemixBase *)mixstream, 0, SEEK_SET);
      n = remix_stream_gain (env, mixstream, n, track->gain);
      
      remix_seek (env, (RemixBase *)mixstream, 0, SEEK_SET);
      remix_seek (env, (RemixBase *)output, output_offset, SEEK_SET);
      n = remix_stream_mix (env, mixstream, output, n);
    }

    processed += n;
    remaining -= n;
  }

  remix_dprintf ("[remix_deck_process] processed %ld\n", processed);

  return processed;
}

static RemixCount
remix_deck_twotrack_process (RemixEnv * env, RemixBase * base, RemixCount count,
                             RemixStream * input, RemixStream * output)
{
  RemixDeck * deck = (RemixDeck *)base;
  CDList * l;
  RemixTrack * track1, * track2;
  RemixCount n;
  RemixCount current_offset = remix_tell (env, base);
  RemixCount input_offset = remix_tell (env, (RemixBase *)input);
  RemixCount output_offset = remix_tell (env, (RemixBase *)output);
  RemixStream * mixstream = deck->_mixstream;

  remix_dprintf ("PROCESS DECK [twotrack] (%p, +%ld, %p -> %p) @ %ld\n",
                 deck, count, input, output, current_offset);

  l = deck->tracks;
  track1 = (RemixTrack *)l->data.s_pointer;

  l = l->next;
  track2 = (RemixTrack *)l->data.s_pointer;

  n = remix_process (env, (RemixBase *)track1, count, input, output);

  remix_seek (env, (RemixBase *)output, output_offset, SEEK_SET);
  n = remix_stream_gain (env, output, n, track1->gain);

  remix_seek (env, (RemixBase *)input, input_offset, SEEK_SET);
  remix_seek (env, (RemixBase *)mixstream, 0, SEEK_SET);
  n = remix_process (env, (RemixBase *)track2, n, input, mixstream);

  remix_seek (env, (RemixBase *)mixstream, 0, SEEK_SET);
  n = remix_stream_gain (env, mixstream, n, track2->gain);

  remix_seek (env, (RemixBase *)mixstream, 0, SEEK_SET);
  remix_seek (env, (RemixBase *)output, output_offset, SEEK_SET);
  n = remix_stream_mix (env, mixstream, output, n);

  remix_dprintf ("[remix_deck_twotrack_process] processed %ld\n", n);

  return n;
}

static RemixCount
remix_deck_onetrack_process (RemixEnv * env, RemixBase * base, RemixCount count,
                             RemixStream * input, RemixStream * output)
{
  RemixDeck * deck = (RemixDeck *)base;
  RemixTrack * track = (RemixTrack *)deck->tracks->data.s_pointer;
  RemixCount n;

  remix_dprintf ("PROCESS DECK [onetrack] (%p, +%ld, %p -> %p) @ %ld\n",
                 deck, count, input, output, remix_tell (env, base));

  n = remix_process (env, (RemixBase *)track, count, input, output);

  remix_dprintf ("*** deck @ %ld\ttrack @ %ld\n", remix_tell (env, base),
                 remix_tell (env, (RemixBase *)track));

  remix_dprintf ("[remix_deck_onetrack_process] processed %ld\n", n);

  return n;
}

static RemixCount
remix_deck_seek (RemixEnv * env, RemixBase * base, RemixCount offset)
{
  RemixDeck * deck = (RemixDeck *)base;
  CDList * l;
  RemixTrack * track;

  for (l = deck->tracks; l; l = l->next) {
    track = (RemixTrack *)l->data.s_pointer;
    remix_seek (env, (RemixBase *)track, offset, SEEK_SET);
  }

  return offset;
}

static int
remix_deck_flush (RemixEnv * env, RemixBase * base)
{
  RemixDeck * deck = (RemixDeck *)base;
  CDList * l;
  RemixTrack * track;

  for (l = deck->tracks; l; l = l->next) {
    track = (RemixTrack *)l->data.s_pointer;
    remix_flush (env, (RemixBase *)track);
  }

  return 0;
}

static struct _RemixMethods _remix_deck_empty_methods = {
  remix_deck_clone,   /* clone */
  remix_deck_destroy, /* destroy */
  remix_deck_ready,   /* ready */
  remix_deck_prepare, /* preapre */
  remix_null_process, /* process */
  remix_null_length,  /* length */
  NULL,            /* seek */
  NULL,            /* flush */
};

static struct _RemixMethods _remix_deck_methods = {
  remix_deck_clone,   /* clone */
  remix_deck_destroy, /* destroy */
  remix_deck_ready,   /* ready */
  remix_deck_prepare, /* preapre */
  remix_deck_process, /* process */
  remix_deck_length,  /* length */
  remix_deck_seek,    /* seek */
  remix_deck_flush,   /* flush */
};

static struct _RemixMethods _remix_deck_onetrack_methods = {
  remix_deck_clone,            /* clone */
  remix_deck_destroy,          /* destroy */
  remix_deck_ready,            /* ready */
  remix_deck_prepare,          /* preapre */
  remix_deck_onetrack_process, /* process */
  remix_deck_length,           /* length */
  remix_deck_seek,             /* seek */
  remix_deck_flush,   /* flush */
};

static struct _RemixMethods _remix_deck_twotrack_methods = {
  remix_deck_clone,            /* clone */
  remix_deck_destroy,          /* destroy */
  remix_deck_ready,            /* ready */
  remix_deck_prepare,          /* preapre */
  remix_deck_twotrack_process, /* process */
  remix_deck_length,           /* length */
  remix_deck_seek,             /* seek */
  remix_deck_flush,   /* flush */
};

static RemixDeck *
remix_deck_optimise (RemixEnv * env, RemixDeck * deck)
{
  int nr_tracks = cd_list_length (env, deck->tracks);

  switch (nr_tracks) {
  case 0: _remix_set_methods (env, deck, &_remix_deck_empty_methods); break;
  case 1: _remix_set_methods (env, deck, &_remix_deck_onetrack_methods); break;
  case 2: _remix_set_methods (env, deck, &_remix_deck_twotrack_methods); break;
  default: _remix_set_methods (env, deck, &_remix_deck_methods); break;
  }

  return deck;
}
