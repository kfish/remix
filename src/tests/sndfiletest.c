/*
 * sndfiletest.c
 *
 * Copyright (C) 2006 Commonwealth Scientific and Industrial Research
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

#include "tests.h"

static void non_existant_file (void);

static void
non_existant_file (void)
{
  RemixEnv * env;
  RemixBase * sf1;
  RemixPlugin * sf_plugin;
  CDSet * sf_parms;
  int sf_path_key;
  CDScalar name;

  INFO ("Attempting to read non existant file") ;
  
  env = remix_init ();
  remix_set_tempo (env, 120);
  remix_set_channels (env, REMIX_STEREO);

  sf_plugin = remix_find_plugin (env, "builtin::sndfile_reader");
  sf_parms = cd_set_new (env);
  sf_path_key = remix_get_init_parameter_key (env, sf_plugin, "path");
  name.s_string = "bad_file_name.wav" ;
  sf_parms = cd_set_insert (env, sf_parms, sf_path_key, name);
  if (sf_plugin == NULL) {
    FAIL ("Newly created sndfile plugin NULL");
  }

  sf1 = remix_new (env, sf_plugin, sf_parms);
}

int
main (int argc, char ** argv)
{
  non_existant_file () ;

  return 0;
}
