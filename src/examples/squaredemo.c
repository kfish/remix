/*
 * squaredemo.c
 *
 * Copyright (C) 2001 Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO), Australia.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include <remix/remix.h>

#define remix_add_sound_B(a,s,l,t1,t2) \
  remix_sound_new ((a), (s), (l), REMIX_BEAT24S(t1), REMIX_BEAT24S(t2))

#define BEAT 24
#define HALF_BEAT 12
#define QUARTER_BEAT 6
#define EIGHTH_BEAT 3

int
main (int argc, char ** argv)
{
  RemixEnv * env;
  RemixDeck * deck, * deck2;
  RemixTrack * track, * track2, * track3, * track4;
  RemixLayer * l1, * l2, * l3, * l4, * l5;
  RemixSound * s;
  RemixSquareTone * square1, * square2, * square3, * square4, * square5;
  RemixPlugin * noise_plugin;
  RemixBase * noise;
  RemixEnvelope * env1, * env2, * env3;
  RemixEnvelope * be1, * be2, * be3;
  RemixCount length;
  RemixMonitor * monitor;
  int b = 0, b2 = 0, mb;

  env = remix_init ();

  remix_set_tempo (env, 160);
  remix_set_channels (env, REMIX_STEREO);

  square1 = remix_squaretone_new (env, 220.0);
  square2 = remix_squaretone_new (env, 440.0);
  square3 = remix_squaretone_new (env, 385.0);
  square4 = remix_squaretone_new (env, 231.0);
  square5 = remix_squaretone_new (env, 165.0);

  noise_plugin = remix_find_plugin (env, "builtin::noise");
  noise = remix_new (env, noise_plugin, NULL);

  monitor = remix_monitor_new (env);

  deck = remix_deck_new (env);

#if 1
  track = remix_track_new (env, deck);

  l1 = remix_layer_new_ontop (env, track, REMIX_TIME_BEAT24S);
  remix_add_sound_B (env, square1, l1, b, BEAT); b += BEAT;
#if 1
  remix_add_sound_B (env, square2, l1, b, QUARTER_BEAT); b += HALF_BEAT;
  remix_add_sound_B (env, square2, l1, b, HALF_BEAT); b += HALF_BEAT;
  remix_add_sound_B (env, square1, l1, b, HALF_BEAT); b += HALF_BEAT;
  remix_add_sound_B (env, square2, l1, b, QUARTER_BEAT); b += HALF_BEAT;
  remix_add_sound_B (env, square3, l1, b, HALF_BEAT); b += HALF_BEAT;
  remix_add_sound_B (env, square2, l1, b, HALF_BEAT); b += HALF_BEAT;
#endif
#endif

#if 1
#if 1
  track2 = remix_track_new (env, deck);
  remix_track_set_gain (env, track2, 0.6);
#else
  track2 = track;
#endif
  l2 = remix_layer_new_ontop (env, track2, REMIX_TIME_BEAT24S);
  s = remix_add_sound_B (env, square4, l2, b2, HALF_BEAT); b2 += BEAT;

  be1 = remix_envelope_new (env, REMIX_ENVELOPE_LINEAR);
  remix_envelope_set_timetype (env, be1, REMIX_TIME_BEAT24S);
  remix_envelope_add_point (env, be1, REMIX_BEAT24S(0), 0.9);
  remix_sound_set_blend_envelope (env, s, be1);

#if 1
  s = remix_add_sound_B (env, square5, l2, b2, HALF_BEAT); b2 += BEAT;
  be2 = remix_envelope_new (env, REMIX_ENVELOPE_LINEAR);
  remix_envelope_set_timetype (env, be2, REMIX_TIME_BEAT24S);
  remix_envelope_add_point (env, be2, REMIX_BEAT24S(0), 0.6);
  remix_sound_set_blend_envelope (env, s, be2);

  s = remix_add_sound_B (env, square4, l2, b2, BEAT); b2 += 48;
  be3 = remix_envelope_new (env, REMIX_ENVELOPE_LINEAR);
  remix_envelope_set_timetype (env, be3, REMIX_TIME_BEAT24S);
  remix_envelope_add_point (env, be3, REMIX_BEAT24S(0), 0.8);
  remix_sound_set_blend_envelope (env, s, be3);
#endif
#endif

#if 1
  track4 = remix_track_new (env, deck);
  remix_track_set_gain (env, track4, 0.1);
  l5 = remix_layer_new_ontop (env, track4, REMIX_TIME_BEAT24S);
  remix_add_sound_B (env, noise, l5, 0, b);
#endif

  mb = MAX (b, b2);

  length = remix_length (env, deck);
  printf ("deck has length %ld\n", length);

  deck2 = remix_deck_new (env);
  track3 = remix_track_new (env, deck2);
  l3 = remix_layer_new_ontop (env, track3, REMIX_TIME_BEAT24S);

  s = remix_add_sound_B (env, deck, l3, 0, mb);

#if 1
  env1 = remix_envelope_new (env, REMIX_ENVELOPE_LINEAR);
  remix_envelope_set_timetype (env, env1, REMIX_TIME_BEAT24S);
  remix_envelope_add_point (env, env1, REMIX_BEAT24S(0), 0.0);
  remix_envelope_add_point (env, env1, REMIX_BEAT24S(mb), 0.9);
  remix_sound_set_gain_envelope (env, s, env1);
#endif

  s = remix_add_sound_B (env, deck, l3, mb, mb);

#if 1
  env2 = remix_envelope_new (env, REMIX_ENVELOPE_LINEAR);
  remix_envelope_set_timetype (env, env2, REMIX_TIME_BEAT24S);
  remix_envelope_add_point (env, env2, REMIX_BEAT24S(0), 0.9);
  remix_sound_set_gain_envelope (env, s, env2);
#endif

  s = remix_add_sound_B (env, deck, l3, 2*mb, mb);

#if 1
  env3 = remix_envelope_new (env, REMIX_ENVELOPE_LINEAR);
  remix_envelope_set_timetype (env, env3, REMIX_TIME_BEAT24S);
  remix_envelope_add_point (env, env3, REMIX_BEAT24S(0), 0.9);
  remix_envelope_add_point (env, env3, REMIX_BEAT24S(mb), 0.0);
  remix_sound_set_gain_envelope (env, s, env3);
#endif

  length = remix_length (env, deck2);

  l4 = remix_layer_new_ontop (env, track3, REMIX_TIME_SAMPLES);
  remix_sound_new (env, monitor, l4, REMIX_SAMPLES(0), REMIX_SAMPLES(length));

  printf ("deck: %p\ttrack: %p\tl1: %p\ttrack2: %p\tl2: %p\n",
          deck, track, l1, track2, l2);
  printf ("deck2: %p\ttrack3: %p\tl3: %p\tl4: %p\n", deck2, track3, l3, l4);
  printf ("square1: %p\tsquare2: %p\tsquare3: %p\nsquare4: %p\tsquare5: %p\n",
          square1, square2, square3, square4, square5);
  printf ("monitor: %p\n", monitor);

  remix_process (env, deck2, length, RemixNone, RemixNone);

  remix_purge (env);

  exit (0);
}
