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
 * remix_channelset: default channel sets.
 *
 * Conrad Parker <conrad@metadecks.org>, August 2001
 */

#define __REMIX__
#include "remix.h"

CDSet * REMIX_MONO = RemixNone;
CDSet * REMIX_STEREO = RemixNone;

void
remix_channelset_defaults_initialise (RemixEnv * env)
{
  if (REMIX_MONO == NULL) {
    REMIX_MONO = cd_set_new (env);
    REMIX_MONO = cd_set_insert (env, REMIX_MONO, REMIX_CHANNEL_LEFT,
			       CD_POINTER(NULL));
  }

  if (REMIX_STEREO == NULL) {
    REMIX_STEREO = cd_set_new (env);
    REMIX_STEREO = cd_set_insert (env, REMIX_STEREO, REMIX_CHANNEL_LEFT,
				 CD_POINTER(NULL));
    REMIX_STEREO = cd_set_insert (env, REMIX_STEREO, REMIX_CHANNEL_RIGHT,
				 CD_POINTER(NULL));
  }

  return;
}

void
remix_channelset_defaults_destroy (RemixEnv * env)
{
  if (REMIX_MONO != NULL) {
    cd_set_free (env, REMIX_MONO);
    REMIX_MONO = NULL;
  }

  if (REMIX_STEREO != NULL) {
    cd_set_free (env, REMIX_STEREO);
    REMIX_STEREO = NULL;
  }
}
