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

int
main (int argc, char ** argv)
{
  RemixEnv * env;

  INFO ("+ Creating new RemixEnv");
  env = remix_init ();

  return 0;
}
