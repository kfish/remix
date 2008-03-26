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
 * RemixContext: REMIX core context.
 *
 * Conrad Parker <conrad@metadecks.org>, August 2001
 */

#include <string.h>

#define __REMIX__
#include "remix.h"

static void
remix_plugin_destroy (RemixEnv * env, RemixPlugin * plugin)
{
  if (plugin->destroy) {
    plugin->destroy (env, plugin);
  }
}

static void
remix_context_destroy (RemixEnv * env)
{
  RemixContext * ctx = env->context;
  RemixWorld * world = env->world;

  world->purging = 1;

  cd_list_apply (env, world->plugins, (CDFunc)remix_plugin_destroy);
  world->plugins = cd_list_free (env, world->plugins);

  /* XXX: remix_destroy_list (env, world->plugins); */
  /* XXX:  remix_destroy_list (env, world->bases); */
  remix_channelset_defaults_destroy (env);
  remix_free (ctx);
  remix_free (world);
}

static RemixEnv *
remix_add_thread_context (RemixContext * ctx, RemixWorld * world)
{
  RemixEnv * env = remix_malloc (sizeof (struct _RemixThreadContext));
  env->context = ctx;
  env->world = world;
  world->refcount++;
  return env;
}

RemixContext *
_remix_context_copy (RemixEnv * env, RemixContext * dest)
{
  RemixContext * ctx = env->context;

  if (dest == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return RemixNone;
  }

  dest->samplerate = ctx->samplerate;
  dest->tempo = ctx->tempo;
  dest->mixlength = ctx->mixlength;
  dest->channels = cd_set_clone_keys (env, ctx->channels);

  return dest;
}

/*
 * _remix_context_merge (env, dest)
 *
 * Merges the context of env into context 'dest'. Copies over the samplerate
 * and tempo, and expands the mixlength and channels if they are greater in
 * 'env's context than in 'dest'.
 */
RemixContext *
_remix_context_merge (RemixEnv * env, RemixContext * dest)
{
  RemixContext * ctx = env->context;
  CDSet * s;
  CDScalar k;

  if (dest == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return RemixNone;
  }

  dest->samplerate = ctx->samplerate;
  dest->tempo = ctx->tempo;

  if (ctx->mixlength > dest->mixlength)
    dest->mixlength = ctx->mixlength;

  for (s = ctx->channels; s; s = s->next) {
    k = cd_set_find (env, dest->channels, s->key);
    if (k.s_pointer == NULL)
      dest->channels = cd_set_insert (env, dest->channels, s->key,
				      CD_POINTER(NULL));
  }

  return dest;
}

/*
 * remix_init ()
 */
RemixEnv *
remix_init (void)
{
  RemixEnv * env;
  RemixWorld * world =
    (RemixWorld *) remix_malloc (sizeof (struct _RemixWorld));
  RemixContext * ctx =
    (RemixContext *) remix_malloc (sizeof (struct _RemixContext));

  world->refcount = 0;
  world->plugins = cd_list_new (ctx);
  world->bases = cd_list_new (ctx);
  world->purging = FALSE;

  ctx->mixlength = REMIX_DEFAULT_MIXLENGTH;
  ctx->samplerate = REMIX_DEFAULT_SAMPLERATE;
  ctx->tempo = REMIX_DEFAULT_TEMPO;

  env = remix_add_thread_context (ctx, world);
  remix_channelset_defaults_initialise (env);
  ctx->channels = REMIX_MONO;

  remix_plugin_defaults_initialise (env);

  return env;
}

/*
 * remix_init_clone ()
 */
RemixEnv *
remix_init_clone (RemixEnv * env)
{
  RemixEnv * new_env = remix_add_thread_context (env->context, env->world);
  return new_env;
}

void
remix_purge (RemixEnv * env)
{
  RemixWorld * world = env->world;
  world->refcount--;
  if (world->refcount <= 0) {
    remix_context_destroy (env);
  }
  remix_free (env);
}

RemixError
remix_set_error (RemixEnv * env, RemixError error)
{
  RemixError old = env->last_error;
  env->last_error = error;
  return old;
}

RemixError
remix_last_error (RemixEnv * env)
{
  return env->last_error;
}

RemixCount
remix_set_mixlength (RemixEnv * env, RemixCount mixlength)
{
  RemixContext * ctx = env->context;
  RemixCount old = ctx->mixlength;
  ctx->mixlength = mixlength;
  return old;
}

RemixCount
remix_get_mixlength (RemixEnv * env)
{
  RemixContext * ctx = env->context;
  return ctx->mixlength;
}

RemixSamplerate
remix_set_samplerate (RemixEnv * env, RemixSamplerate samplerate)
{
  RemixContext * ctx = env->context;
  RemixSamplerate old = ctx->samplerate;
  ctx->samplerate = samplerate;
  return old;
}

RemixSamplerate
remix_get_samplerate (RemixEnv * env)
{
  RemixContext * ctx = env->context;
  return ctx->samplerate;
}

RemixTempo
remix_set_tempo (RemixEnv * env, RemixTempo tempo)
{
  RemixContext * ctx = env->context;
  RemixTempo old = ctx->tempo;
  ctx->tempo = tempo;
  return old;
}

RemixTempo
remix_get_tempo (RemixEnv * env)
{
  RemixContext * ctx = env->context;
  return ctx->tempo;
}

CDSet *
remix_set_channels (RemixEnv * env,  CDSet * channels)
{
  RemixContext * ctx = env->context;
  CDSet * old = ctx->channels;
  ctx->channels = cd_set_clone_keys (env, channels);
  return old;
}

CDSet *
remix_get_channels (RemixEnv * env)
{
  RemixContext * ctx = env->context;
  return ctx->channels;
}

RemixEnv *
_remix_register_plugin (RemixEnv * env, RemixPlugin * plugin)
{
  RemixWorld * world = env->world;
  remix_dprintf ("[_remix_register_plugin] REGISTERING %s\n",
		 plugin->metatext ? plugin->metatext->identifier : "(\?\?\?)");
  world->plugins = cd_list_append (env, world->plugins, CD_POINTER(plugin));
  return env;
}

RemixEnv *
_remix_unregister_plugin (RemixEnv * env, RemixPlugin * plugin)
{
  RemixWorld * world = env->world;
  if (world->purging) return env;

  world->plugins = cd_list_remove (env, world->plugins, CD_TYPE_POINTER,
				   CD_POINTER(plugin));
  return env;
}

RemixEnv *
_remix_register_base (RemixEnv * env, RemixBase * base)
{
  RemixWorld * world = env->world;
  world->bases = cd_list_append (env, world->bases, CD_POINTER(base));
  return env;
}

RemixEnv *
_remix_unregister_base (RemixEnv * env, RemixBase * base)
{
  RemixWorld * world = env->world;
  if (world->purging) return env;

  world->bases = cd_list_remove (env, world->bases, CD_TYPE_POINTER,
				 CD_POINTER(base));
  return env;
}

static int
plugin_id_eq (RemixEnv * env, RemixPlugin * plugin, char * identifier)
{
  if (plugin == RemixNone) return FALSE;
  if (plugin->metatext == RemixNone) return FALSE;
  return !(strcmp(plugin->metatext->identifier, identifier));
}

RemixPlugin *
remix_find_plugin (RemixEnv * env, char * identifier)
{
  RemixWorld * world = env->world;
  CDList * l = cd_list_find_first (env, world->plugins, CD_TYPE_POINTER,
				   (CDCmpFunc)plugin_id_eq,
				   CD_POINTER(identifier));

  if (l == RemixNone) return RemixNone;
  return (RemixPlugin *)l->data.s_pointer;
}
