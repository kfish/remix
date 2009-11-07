/*
 * sndfiledemo.c
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

#include <stdlib.h>
#include <stdio.h>

#include <remix/remix.h>

int
main (int argc, char ** argv)
{
  RemixEnv * env;
  RemixDeck * deck, * deck2;
  RemixTrack * track, * track2, * track3;
  RemixLayer * l1, * l2, * l3, * l4, * l5;
  RemixBase * sf1, * sf2;
  RemixSquareTone * square;
  RemixCount length;
  RemixMonitor * monitor;
  RemixSound * sm;
  RemixEnvelope * envelope;
  RemixPlugin * gain_plugin;
  RemixPlugin * sf_plugin;
  RemixBase * gain;
  CDSet * sf_parms;
  int i;
  int gain_key;
  int sf_path_key;

  env = remix_init ();
  remix_set_tempo (env, 140);
  remix_set_channels (env, REMIX_STEREO);

#if 0
  sf1 = remix_sndfile_new (env, "./1052.wav", 0);
  sf2 = remix_sndfile_new (env, "./909_cl.wav", 0);
#else
  sf_plugin = remix_find_plugin (env, "builtin::sndfile_reader");
  sf_parms = cd_set_new (env);
  sf_path_key = remix_get_init_parameter_key (env, sf_plugin, "path");
  sf_parms = cd_set_insert (env, sf_parms, sf_path_key, CD_STRING(SAMPLEDIR "/1052.wav"));
  if (sf_plugin == NULL) {
    fprintf (stderr, "sf_plugin == NULL\n");
    exit (1);
  }
  sf1 = remix_new (env, sf_plugin, sf_parms);
  sf_parms = cd_set_replace (env, sf_parms, 1, CD_STRING(SAMPLEDIR "/909_cl.wav"));
  sf2 = remix_new (env, sf_plugin, sf_parms);
#endif
  square = remix_squaretone_new (env, 220);
  monitor = remix_monitor_new (env);

#if 0
  gain = remix_gain_new (env);
#else
  gain_plugin = remix_find_plugin (env, "builtin::gain");
  gain = remix_new (env, gain_plugin, NULL);
#endif

  envelope = remix_envelope_new (env, REMIX_ENVELOPE_LINEAR);
  remix_envelope_set_timetype (env, envelope, REMIX_TIME_BEAT24S);
  remix_envelope_add_point (env, envelope, REMIX_BEAT24S(0), 0.1);
  remix_envelope_add_point (env, envelope, REMIX_BEAT24S(48), 1.3);
  remix_envelope_add_point (env, envelope, REMIX_BEAT24S(96), 0.1);

#if 0
  remix_gain_set_envelope (env, gain, env);
#else
  gain_key = remix_get_parameter_key (env, gain, "Gain envelope");
  printf ("got gain key: %d\n", gain_key);
  remix_set_parameter (env, gain, gain_key, (CDScalar)((void *)envelope));
#endif

  deck = remix_deck_new (env);
  track = remix_track_new (env, deck);

  l1 = remix_layer_new_ontop (env, track, REMIX_TIME_BEAT24S);
  for (i=0; i < 4; i++) {
    remix_sound_new (env, (RemixBase *)sf1, l1, REMIX_BEAT24S(i*24), REMIX_BEAT24S(18));
  }

  track2 = remix_track_new (env, deck);
  l2 = remix_layer_new_ontop (env, track2, REMIX_TIME_BEAT24S);
  for (i=0; i < 16; i++) {
    remix_sound_new (env, (RemixBase *)sf2, l2, REMIX_BEAT24S(i*6), REMIX_BEAT24S(4));
  }

  deck2 = remix_deck_new (env);
  track3 = remix_track_new (env, deck2);

  l3 = remix_layer_new_ontop (env, track3, REMIX_TIME_BEAT24S);
  remix_sound_new (env, (RemixBase *)deck, l3, REMIX_BEAT24S(0), REMIX_BEAT24S(96));

#if 1
  l4 = remix_layer_new_ontop (env, track3, REMIX_TIME_BEAT24S);
  remix_sound_new (env, (RemixBase *)gain, l4, REMIX_BEAT24S(0), REMIX_BEAT24S(96));
#endif

  l5 = remix_layer_new_ontop (env, track3, REMIX_TIME_SAMPLES);

  length = remix_length (env, (RemixBase *)deck);
  sm = remix_sound_new (env, monitor, l5, REMIX_SAMPLES(0), REMIX_SAMPLES(length));

  printf ("deck: %p\ttrack: %p\tl1: %p\nl3: %p\tmonitor: %p\n",
          deck, track, l1, l3, monitor);

  remix_process (env, deck2, length, RemixNone, RemixNone);

  remix_set_tempo (env, 105);
  length = remix_length (env, (RemixBase *)deck);
  remix_sound_set_duration (env, sm, REMIX_SAMPLES(length));
  remix_seek (env, deck2, 0, SEEK_SET);
  remix_process (env, deck2, length, RemixNone, RemixNone);

  remix_set_tempo (env, 168);
  length = remix_length (env, (RemixBase *)deck);
  remix_sound_set_duration (env, sm, REMIX_SAMPLES(length));
  remix_seek (env, deck2, 0, SEEK_SET);
  remix_process (env, deck2, length, RemixNone, RemixNone);

  remix_purge (env);

  exit (0);
}
