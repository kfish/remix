/*
 * libremix -- An audio mixing and sequencing library.
 *
 * Copyright (C) 2001 Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO), Australia.
 * Copyright (C) 2003 Conrad Parker
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
 * Conrad Parker <conrad@metadecks.org>, August 2001
 */

#ifndef __REMIX_DECK_H__
#define __REMIX_DECK_H__

/** \file
 *
 * The top level structured mixing abstraction in libremix is known as a deck.
 * A deck contains a number of tracks which are mixed in parallel. Each
 * track may contain a number of layers which are mixed from bottom
 * to top in series. Finally, these layers each contain a sequence of
 * sounds with transparency.
 *
 * \image html decks.png
 * \image latex decks.eps "Inside a Remix deck" width=10cm
 *
 * The sequence of sounds in a layer can be indexed by samples, seconds or
 * tempo.
 * Sounds provide audio data from any instrument or effect source, and these
 * sources can each be reused multiple times. A sound can even source its
 * audio data from another entire deck, thus decks can be used to sequence
 * other decks. In this manner effects can be applied to sequences of decks,
 * and sequences of decks can be stored as higher level units such as
 * verses and choruses in a music application.
 */


#include <remix/remix_types.h>

#if defined(__cplusplus)
extern "C" {
#endif

/* Decks */
RemixDeck * remix_deck_new (RemixEnv * env);

RemixCount remix_deck_set_mixlength (RemixEnv * env, RemixDeck * deck, RemixCount mixlength);
RemixCount remix_deck_get_mixlength (RemixEnv * env, RemixDeck * deck);

CDList * remix_deck_get_tracks (RemixEnv * env, RemixDeck * deck);

/* Tracks */
RemixTrack * remix_track_new (RemixEnv * env, RemixDeck * deck);
RemixPCM remix_track_set_gain (RemixEnv * env, RemixTrack * track, RemixPCM gain);
RemixPCM remix_track_get_gain (RemixEnv * env, RemixTrack * track);
RemixCount remix_track_set_mixlength (RemixEnv * env, RemixTrack * track,
				RemixCount mixlength);
RemixCount remix_track_get_mixlength (RemixEnv * env, RemixTrack * track);
void remix_remove_track (RemixEnv * env, RemixTrack * track);

RemixDeck * remix_track_get_deck (RemixEnv * env, RemixTrack * track);


/* Layers */
RemixLayer * remix_layer_new_ontop (RemixEnv * env, RemixTrack * track,
				    RemixTimeType timetype);
RemixLayer * remix_layer_new_above (RemixEnv * env, RemixLayer * above,
				    RemixTimeType timetype);
RemixLayer * remix_layer_move_ontop (RemixEnv * env, RemixLayer * layer, RemixTrack * track);
RemixLayer * remix_layer_move_above (RemixEnv * env, RemixLayer * layer, RemixLayer * above);
RemixLayer * remix_layer_raise (RemixEnv * env, RemixLayer * layer);
RemixLayer * remix_layer_lower (RemixEnv * env, RemixLayer * layer);
RemixTrack * remix_layer_get_track (RemixEnv * env, RemixLayer * layer);
RemixDeck * remix_layer_get_deck (RemixEnv * env, RemixLayer * layer);
RemixTimeType remix_layer_set_timetype (RemixEnv * env, RemixLayer * layer,
				  RemixTimeType new_type);
RemixTimeType remix_layer_get_timetype (RemixEnv * env, RemixLayer * layer);
RemixSound * remix_layer_get_sound_before (RemixEnv * env, RemixLayer * layer,
				     RemixTime time);
RemixSound * remix_layer_get_sound_at (RemixEnv * env, RemixLayer * layer, RemixTime time);
RemixSound * remix_layer_get_sound_after (RemixEnv * env, RemixLayer * layer, RemixTime time);
RemixLayer * remix_layer_below (RemixEnv * env, RemixLayer * layer);
RemixLayer * remix_layer_above (RemixEnv * env, RemixLayer * layer);

/* Sounds */
RemixSound * remix_sound_new (RemixEnv * env, RemixBase * source,
			      RemixLayer * layer,
			      RemixTime start_time, RemixTime duration);

RemixBase * remix_sound_set_source (RemixEnv * env, RemixSound * sound,
				    RemixBase * source);
RemixBase * remix_sound_get_source (RemixEnv * env, RemixSound * sound);
RemixLayer * remix_sound_get_layer (RemixEnv * env, RemixSound * sound);
RemixTrack * remix_sound_get_track (RemixEnv * env, RemixSound * sound);
RemixDeck * remix_sound_get_deck (RemixEnv * env, RemixSound * sound);

RemixTime remix_sound_move (RemixEnv * env, RemixSound * sound,
			    RemixTime time);
RemixSound * remix_sound_get_prev (RemixEnv * env, RemixSound * sound);
RemixSound * remix_sound_get_next (RemixEnv * env, RemixSound * sound);

RemixTime remix_sound_set_start_time (RemixEnv * env, RemixSound * sound,
				      RemixTime time);
RemixTime remix_sound_get_start_time (RemixEnv * env, RemixSound * sound);
RemixTime remix_sound_set_duration (RemixEnv * env, RemixSound * sound,
				    RemixTime time);
RemixTime remix_sound_get_duration (RemixEnv * env, RemixSound * sound);

RemixBase * remix_sound_set_rate_envelope (RemixEnv * env, RemixSound * sound,
					   RemixBase * rate_envelope);
RemixBase * remix_sound_get_rate_envelope (RemixEnv * env, RemixSound * sound);
RemixBase * remix_sound_set_gain_envelope (RemixEnv * env, RemixSound * sound,
					   RemixBase * gain_envelope);
RemixBase * remix_sound_get_gain_envelope (RemixEnv * env, RemixSound * sound);
RemixBase * remix_sound_set_blend_envelope (RemixEnv * env, RemixSound * sound,
					    RemixBase * blend_envelope);
RemixBase * remix_sound_get_blend_envelope (RemixEnv * env, RemixSound * sound);

#if defined(__cplusplus)
}
#endif

#endif /* __REMIX_DECK_H__ */
