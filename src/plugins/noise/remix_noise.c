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
 * RemixNoise: a noise generator
 *
 * Conrad Parker <Conrad.Parker@CSIRO.AU>, August 2001
 */

#include <stdlib.h>

#define __REMIX_PLUGIN__
#include <remix/remix.h>

/* Optimisation dependencies: none */
static RemixBase * remix_noise_optimise (RemixEnv * env, RemixBase * noise);

static RemixBase *
remix_noise_init (RemixEnv * env, RemixBase * base, CDSet * parameters)
{
  remix_noise_optimise (env, base);
  return base;
}

static RemixBase *
remix_noise_clone (RemixEnv * env, RemixBase * base)
{
  RemixBase * new_noise = remix_base_new (env);
  remix_noise_optimise (env, new_noise);
  return new_noise;
}

static int
remix_noise_destroy (RemixEnv * env, RemixBase * base)
{
  free (base);
  return 0;
}

/* An RemixChunkFunc for creating noise */
static RemixCount
remix_noise_write_chunk (RemixEnv * env, RemixChunk * chunk, RemixCount offset,
		      RemixCount count, int channelname, void * data)
{
  RemixPCM * d;
  RemixPCM r, rmax = (RemixPCM)RAND_MAX;
  RemixCount i;

  remix_dprintf ("[remix_noise_write_chunk] (%p, +%ld) @ %ld\n", data, count,
	      offset);

  d = &chunk->data[offset];
  for (i = 0; i < count; i++) {
    r = (RemixPCM) random ();
    r *= 2.0;
    r /= rmax;
    *d++ = r - 1.0;
  }

  return count;
}

static RemixCount
remix_noise_process (RemixEnv * env, RemixBase * base, RemixCount count,
		  RemixStream * input, RemixStream * output)
{
  return remix_stream_chunkfuncify (env, output, count,
				 remix_noise_write_chunk, base);
}

static RemixCount
remix_noise_length (RemixEnv * env, RemixBase * base)
{
  return REMIX_COUNT_INFINITE;
}

static struct _RemixMethods _remix_noise_methods = {
  remix_noise_clone,
  remix_noise_destroy,
  NULL, /* ready */
  NULL, /* prepare */
  remix_noise_process,
  remix_noise_length,
  NULL, /* seek */
  NULL, /* flush */
};

static RemixBase *
remix_noise_optimise (RemixEnv * env, RemixBase * noise)
{
  remix_base_set_methods (env, noise, &_remix_noise_methods);
  return noise;
}

static struct _RemixMetaText noise_metatext = {
  "envstd::noise",
  "Generators::Noise",
  "White noise generator",
  "Copyright (C) 2001 CSIRO Australia",
  "http://www.metadecks.org/remix/plugins/noise.html",
  REMIX_ONE_AUTHOR ("Conrad Parker", "Conrad.Parker@CSIRO.AU"),
};

static struct _RemixPlugin noise_plugin = {
  &noise_metatext,
  REMIX_FLAGS_NONE,
  CD_EMPTY_SET, /* new scheme */
  remix_noise_init,
  CD_EMPTY_SET, /* process scheme */
  NULL, /* suggests */
  NULL, /* plugin data */
};

CDList *
remix_load (RemixEnv * env)
{
  CDList * plugins = cd_list_new (env);

  plugins = cd_list_prepend (env, plugins, CD_POINTER(&noise_plugin));

  return plugins;
}
