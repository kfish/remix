/*
 * noisedemo.c
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

int
main (int argc, char ** argv)
{
  RemixEnv * env;
  RemixDeck * deck;
  RemixTrack * track;
  RemixLayer * l1, * l2;
  RemixPlugin * noise_plugin;
  RemixBase * noise1, * noise2;
  RemixCount length;
  RemixMonitor * monitor;
  int i;

  env = remix_init ();
  remix_set_channels (env, REMIX_MONO);

  deck = remix_deck_new (env);
  track = remix_track_new (env, deck);

  noise_plugin = remix_find_plugin (env, "envstd::noise");

  if (noise_plugin == NULL) {
    fprintf (stderr, "Noise plugin not found. You must do 'make install' to install it.\n");
    exit (1);
  }

  noise1 = remix_new (env, noise_plugin, NULL);
  noise2 = remix_new (env, noise_plugin, NULL);

  l1 = remix_layer_new_ontop (env, track, REMIX_TIME_SAMPLES);
  for (i=0; i < 10; i++) {
    remix_sound_new (env, noise1, l1, REMIX_SAMPLES(i*2500),
		     REMIX_SAMPLES(1250));
  }

  length = remix_length (env, deck);

  monitor = remix_monitor_new (env);

  l2 = remix_layer_new_ontop (env, track, REMIX_TIME_SAMPLES);
  remix_sound_new (env, monitor, l2, REMIX_SAMPLES(0), REMIX_SAMPLES(length));

  remix_process (env, deck, length, RemixNone, RemixNone);

  remix_purge (env);

  exit (0);
}
