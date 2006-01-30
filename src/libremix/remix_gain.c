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
 * RemixGain: a gain filter
 *
 * Conrad Parker <Conrad.Parker@CSIRO.AU>, August 2001
 */

#include <stdlib.h>

#define __REMIX_PLUGIN__
#include "remix.h"

#define GAIN_ENVELOPE_KEY 1

typedef struct _RemixGain RemixGain;

struct _RemixGain {
  RemixStream * _gain_envstream;
};

/* Optimisation dependencies: none */
static RemixBase * remix_gain_optimise (RemixEnv * env, RemixBase * gain);

static void
remix_gain_replace_mixstream (RemixEnv * env, RemixBase * gain)
{
  RemixCount mixlength = remix_base_get_mixlength (env, gain);
  RemixGain * gi = (RemixGain *)remix_base_get_instance_data (env, gain);

  if (gi->_gain_envstream != RemixNone)
    remix_destroy (env, (RemixBase *)gi->_gain_envstream);

  gi->_gain_envstream =
    remix_stream_new_contiguous (env, mixlength);
}

static RemixBase *
remix_gain_init (RemixEnv * env, RemixBase * base, CDSet * parameters)
{
  remix_base_set_instance_data (env, base,
			       calloc (1, sizeof (struct _RemixGain)));
  remix_gain_replace_mixstream (env, base);
  remix_gain_optimise (env, base);
  return base;
}

static RemixBase *
remix_gain_clone (RemixEnv * env, RemixBase * base)
{
  RemixBase * new_gain = remix_base_new (env);
  remix_gain_init (env, new_gain, CD_EMPTY_SET);
  remix_gain_optimise (env, new_gain);
  return (RemixBase *)new_gain;
}

static int
remix_gain_destroy (RemixEnv * env, RemixBase * base)
{
  free (remix_base_get_instance_data (env, base));
  free (base);
  return 0;
}

static int
remix_gain_ready (RemixEnv * env, RemixBase * base)
{
  return (remix_base_encompasses_mixlength (env, base) &&
	  remix_base_encompasses_channels (env, base));
}

static RemixBase *
remix_gain_prepare (RemixEnv * env, RemixBase * base)
{
  remix_gain_replace_mixstream (env, base);
  return base;
}

static RemixCount
remix_gain_process (RemixEnv * env, RemixBase * base, RemixCount count,
		 RemixStream * input, RemixStream * output)
{
  RemixCount remaining = count, processed = 0, n;
  RemixCount output_offset;
  RemixCount mixlength = remix_base_get_mixlength (env, base);
  RemixBase * gain_envelope;
  RemixGain * gi = remix_base_get_instance_data (env, base);

  remix_dprintf ("PROCESS GAIN (%p, +%ld) @ %ld\n", base, count,
	      remix_tell (env, base));

  gain_envelope =
    (RemixBase *) (remix_get_parameter (env, base, GAIN_ENVELOPE_KEY)).s_pointer;

  if (gain_envelope == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOOP);
    return -1;
  }

  while (remaining > 0) {
    n = MIN (remaining, mixlength);

    output_offset = remix_tell (env, (RemixBase *)output);
    n = remix_stream_copy (env, input, output, n);

    remix_seek (env, (RemixBase *)gi->_gain_envstream, 0, SEEK_SET);
    n = remix_process (env, gain_envelope, n, RemixNone,
		    gi->_gain_envstream);

    remix_seek (env, (RemixBase *)gi->_gain_envstream, 0, SEEK_SET);
    remix_seek (env, (RemixBase *)output, output_offset, SEEK_SET);
    n = remix_stream_mult (env, gi->_gain_envstream, output, n);

    remaining -= n;
    processed += n;
  }

  remix_dprintf ("[remix_gain_process] processed %ld\n", processed);

  return processed;
}

static RemixCount
remix_gain_length (RemixEnv * env, RemixBase * base)
{
  RemixBase * gain_envelope =
    (RemixBase *) (remix_get_parameter (env, base, GAIN_ENVELOPE_KEY)).s_pointer;

  if (gain_envelope == RemixNone) {
    return REMIX_COUNT_INFINITE;
  }

  return remix_length (env, gain_envelope);
}

static RemixCount
remix_gain_seek (RemixEnv * env, RemixBase * base, RemixCount offset)
{
  RemixBase * gain_envelope =
    (RemixBase *) (remix_get_parameter (env, base, GAIN_ENVELOPE_KEY)).s_pointer;

  return remix_seek (env, (RemixBase *)gain_envelope, offset, SEEK_SET);
}

static struct _RemixMethods _remix_gain_methods = {
  remix_gain_clone,
  remix_gain_destroy,
  remix_gain_ready,
  remix_gain_prepare,
  remix_gain_process,
  remix_gain_length,
  remix_gain_seek,
};

static RemixBase *
remix_gain_optimise (RemixEnv * env, RemixBase * gain)
{
  remix_base_set_methods (env, gain, &_remix_gain_methods);
  return gain;
}

static int
remix_gain_plugin_destroy (RemixEnv * env, RemixPlugin * plugin)
{
  cd_set_free (env, plugin->process_scheme);
  return 0;
}

static struct _RemixParameterScheme gain_envelope_scheme = {
  "Gain envelope",
  "An envelope to control the amplitude",
  REMIX_TYPE_BASE,
  REMIX_CONSTRAINT_TYPE_NONE,
  REMIX_CONSTRAINT_EMPTY,
  REMIX_HINT_DEFAULT,
};

static struct _RemixMetaText gain_metatext = {
  "builtin::gain",
  "Processors::Gain Adjustment",
  "Adjusts the gain of its input",
  "Copyright (C) 2001 CSIRO Australia",
  "http://www.metadecks.org/env/plugins/gain.html",
  REMIX_ONE_AUTHOR ("Conrad Parker", "Conrad.Parker@CSIRO.AU"),
};

static struct _RemixPlugin gain_plugin = {
  &gain_metatext,
  REMIX_FLAGS_NONE,
  CD_EMPTY_SET, /* new scheme */
  remix_gain_init,
  CD_EMPTY_SET, /* process_scheme */
  NULL, /* suggests */
  NULL, /* plugin_data */
  remix_gain_plugin_destroy /* destroy */
};

/* module init function */
CDList *
__gain_init (RemixEnv * env)
{
  CDList * plugins = cd_list_new (env);

  gain_plugin.process_scheme =
    cd_set_insert (env, gain_plugin.process_scheme, GAIN_ENVELOPE_KEY,
		   CD_POINTER(&gain_envelope_scheme));

  plugins = cd_list_prepend (env, plugins, CD_POINTER(&gain_plugin));

  return plugins;
}
