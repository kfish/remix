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
 * RemixBase: A generic interface for sound generation and processing.
 *
 * Conrad Parker <Conrad.Parker@CSIRO.AU>, August 2001
 */

#include <string.h>

#define __REMIX__
#include "remix.h"


RemixBase *
remix_new (RemixEnv * env, RemixPlugin * plugin, CDSet * parameters)
{
  RemixBase * base;

  if (plugin == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return RemixNone;
  }

  base = remix_base_new (env);

  if (base == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return RemixNone;
  }

  base->plugin = plugin;

  if (plugin->init != NULL) {
    if (plugin->init (env, base, parameters) == RemixNone) {
      remix_destroy (env, base);
      return RemixNone;
    }
  }

  return base;
}

CDSet *
remix_suggest (RemixEnv * env, RemixPlugin * plugin, CDSet * parameters)
{
  if (plugin == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return RemixNone;
  }

  if (!plugin->suggest) {
    /* XXX: should remix_set_error: is NOOP appropriate ?? */
    return RemixNone;
  }

  return plugin->suggest (env, plugin, parameters, plugin->plugin_data);
}

static int
remix_parameter_scheme_get_key (RemixEnv * env, CDSet * scheme_set, char * name)
{
  CDSet * s;
  RemixParameterScheme * scheme;

  for (s = scheme_set; s; s = s->next) {
    scheme = (RemixParameterScheme *)s->data.s_pointer;
    if (!strcmp (scheme->name, name)) return s->key;
  }

  remix_set_error (env, REMIX_ERROR_NOENTITY);

  return -1;
}

int
remix_get_init_parameter_key (RemixEnv * env, RemixPlugin * plugin, char * name)
{
  if (plugin == RemixNone) {
    remix_dprintf ("[remix_get_init_parameter_key] plugin == RemixNone\n");
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  return remix_parameter_scheme_get_key (env, plugin->init_scheme, name);
}

int
remix_get_parameter_key (RemixEnv * env, RemixBase * base, char * name)
{
  if (base == RemixNone) {
    remix_dprintf ("[remix_get_parameter_key] base == RemixNone\n");
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  if (base->plugin == RemixNone) {
    remix_dprintf ("[remix_get_parameter_key] base->plugin == RemixNone\n");
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  return remix_parameter_scheme_get_key (env, base->plugin->process_scheme, name);
}

RemixParameter
remix_set_parameter (RemixEnv * env, RemixBase * base, int key,
		    RemixParameter parameter)
{
  if (base == RemixNone) {
    remix_dprintf ("[remix_set_parameter] base == RemixNone\n");
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return (RemixParameter)-1;
  }

  remix_dprintf ("[remix_set_parameter] base %p, [%d] ==> %p\n", base, key,
	      parameter.s_pointer);
  base->parameters = cd_set_replace (env, base->parameters, key, parameter);
  return parameter;
}

RemixParameter
remix_get_parameter (RemixEnv * env, RemixBase * base, int key)
{
  RemixParameter p;

  if (base == RemixNone) {
    remix_dprintf ("[remix_get_parameter] base == RemixNone\n");
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return (RemixParameter)-1;
  }

  p = cd_set_find (env, base->parameters, key);
  remix_dprintf ("[remix_get_parameter] base %p, [%d] == %p\n", base, key,
	      p.s_pointer);
  return p;
}

RemixParameterType
remix_get_parameter_type (RemixEnv * env, RemixBase * base, int key)
{
  RemixPlugin * plugin;
  CDScalar k;
  RemixParameterScheme * scheme;

  if (base == RemixNone) {
    remix_dprintf ("[remix_get_parameter_type] base == RemixNone\n");
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  plugin = base->plugin;

  if (plugin == RemixNone) {
    remix_dprintf ("[remix_get_parameter_type] base->plugin == RemixNone\n");
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  k = cd_set_find (env, plugin->process_scheme, key);
  scheme = (RemixParameterScheme *)k.s_pointer;

  if (scheme == RemixNone) {
    remix_dprintf ("[remix_get_parameter_type] scheme == RemixNone\n");
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  return scheme->type;
}

RemixBase *
remix_base_new_subclass (RemixEnv * env, size_t size)
{
  RemixBase * base = remix_malloc (size);
  _remix_context_copy (env, &base->context_limit);
  _remix_register_base (env, base);
  return base;
}

RemixBase *
remix_base_new (RemixEnv * env)
{
  return remix_base_new_subclass (env, sizeof (struct _RemixBase));
}

RemixCount
remix_base_get_mixlength (RemixEnv * env, RemixBase * base)
{
  if (base == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  return _remix_base_get_mixlength (env, base);
}

RemixSamplerate
remix_base_get_samplerate (RemixEnv * env, RemixBase * base)
{
  if (base == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  return _remix_base_get_samplerate (env, base);
}

RemixTempo
remix_base_get_tempo (RemixEnv * env, RemixBase * base)
{
  if (base == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  return _remix_base_get_tempo (env, base);
}

CDSet *
remix_base_get_channels (RemixEnv * env, RemixBase * base)
{
  if (base == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return RemixNone;
  }

  return _remix_base_get_channels (env, base);
}

void *
remix_base_set_instance_data (RemixEnv * env, RemixBase * base, void * data)
{
  if (base == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return RemixNone;
  }

  return _remix_set_instance_data (env, base, data);
}

void *
remix_base_get_instance_data (RemixEnv * env, RemixBase * base)
{
  if (base == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return RemixNone;
  }

  return _remix_get_instance_data (env, base);
}

int
remix_base_has_samplerate (RemixEnv * env, RemixBase * base)
{
  RemixSamplerate asr, bsr;

  asr = remix_get_samplerate (env);
  bsr = _remix_base_get_samplerate (env, base);

  return (asr == bsr);
}

int
remix_base_has_tempo (RemixEnv * env, RemixBase * base)
{
  RemixTempo at, bt;

  at = remix_get_tempo (env);
  bt = _remix_base_get_tempo (env, base);

  return (at == bt);
}

int
remix_base_encompasses_mixlength (RemixEnv * env, RemixBase * base)
{
  RemixCount aml, bml;

  aml = remix_get_mixlength (env);
  bml = _remix_base_get_mixlength (env, base);

  return (aml < bml);
}

int
remix_base_encompasses_channels (RemixEnv * env, RemixBase * base)
{
  CDSet * s, * as, * bs;
  CDScalar k;

  as = remix_get_channels (env);
  bs = _remix_base_get_channels (env, base);

  for (s = as; s; s = s->next) {
    k = cd_set_find (env, bs, s->key);
    if (k.s_pointer == NULL) return 0;
  }

  return 1;
}

RemixMethods *
remix_base_set_methods (RemixEnv * env, RemixBase * base, RemixMethods * methods)
{
  RemixMethods * old = base->methods;
  _remix_set_methods (env, base, methods);
  return old;
}

RemixMethods *
remix_base_get_methods (RemixEnv * env, RemixBase * base)
{
  return _remix_get_methods (env, base);
}

RemixPlugin *
remix_base_set_plugin (RemixEnv * env, RemixBase * base, RemixPlugin * plugin)
{
  RemixPlugin * old = base->plugin;
  _remix_set_plugin (env, base, plugin);
  return old;
}

RemixPlugin *
remix_base_get_plugin (RemixEnv * env, RemixBase * base)
{
  return _remix_get_plugin (env, base);
}

RemixBase *
remix_clone_subclass (RemixEnv * env, RemixBase * base)
{
  if (!base) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return NULL;
  }
  if (!base->methods || !base->methods->clone) {
    remix_set_error (env, REMIX_ERROR_INVALID);
    return NULL;
  }
  return _remix_clone (env, base);
}

int
remix_destroy (RemixEnv * env, RemixBase * base)
{
  if (!base) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  _remix_unregister_base (env, base);

  if (!base->methods || !base->methods->destroy) {
    remix_set_error (env, REMIX_ERROR_INVALID);
    return -1;
  }
  return _remix_destroy (env, base);
}

int
remix_destroy_list (RemixEnv * env, CDList * list)
{
  cd_list_destroy_with (env, list, (CDDestroyFunc)remix_destroy);
  return 0;
}

/*
 * Prepare the methods for process, seek and length calls.
 *
 * "Prepare" means to make sure the base has enough internal buffers
 * to deal with the current context (sample rate, mixlength).
 *
 * If the methods has a ready() function, that is checked first. If it
 * does not have a ready() function, it is assumed to always be ready.
 */
RemixBase *
remix_prepare (RemixEnv * env, RemixBase * base)
{
  int is_ready = 0;

  if (!base) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return NULL;
  }
  if (base->methods && base->methods->prepare) {
    if (base->methods->ready) {
      is_ready = base->methods->ready (env, base);
    }

    _remix_context_merge (env, &base->context_limit);

    if (!is_ready) base =_remix_prepare (env, base);
  }

  return base;
}

RemixCount
remix_process_fast (RemixEnv * env, RemixBase * base, RemixCount count,
		   RemixStream * input, RemixStream * output)
{
  RemixCount n;

  if (!base) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }
  if (!base->methods || !base->methods->process) {
    remix_set_error (env, REMIX_ERROR_INVALID);
    return -1;
  }
  n = _remix_process (env, base, count, input, output);
  if (n > 0) base->offset += n;
  return n;
}

RemixCount
remix_process (RemixEnv * env, RemixBase * base, RemixCount count,
	      RemixStream * input, RemixStream * output)
{
  RemixCount processed;
  RemixCount n;
  RemixError error;
  char * str;

  remix_debug_down ();

  processed = remix_process_fast (env, base, count, input, output);

  if (processed == -1) {
    error = remix_last_error (env);
    str = remix_error_string (env, error);
    remix_dprintf ("*** ERROR in remix_process: %s\n", str);
    switch (error) {
    case REMIX_ERROR_NOOP:
      n = remix_stream_write (env, output, count, input);
      if (n > 0) {
	base->offset += n;
	processed = n;
      }
      break;
    case REMIX_ERROR_SILENCE:
      n = remix_stream_write0 (env, output, count);
      if (n > 0) {
	base->offset += n;
	processed = n;
      }
      break;
    default: break;
    }
  }

  remix_debug_up ();

  return processed;
}

RemixCount
remix_length (RemixEnv * env, RemixBase * base)
{
  if (!base) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }
  if (!base->methods || !base->methods->length) {
    remix_set_error (env, REMIX_ERROR_INVALID);
    return -1;
  }
  return _remix_length (env, base);
}  

RemixCount
remix_seek (RemixEnv * env, RemixBase * base, RemixCount offset, int whence)
{
  RemixCount new_offset, len;

  if (!base) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  new_offset = base->offset;

  switch (whence) {
  case SEEK_SET: new_offset = offset; break;
  case SEEK_CUR: new_offset += offset; break;
  case SEEK_END:
    len = remix_length (env, base);
    if (len == -1) return -1;
    new_offset = len + offset;
    break;
  default:
    remix_set_error (env, REMIX_ERROR_INVALID);
    return -1;
    break;
  }

  if (new_offset == base->offset) return new_offset;

  remix_dprintf ("SEEK %p @ %ld\n", base, new_offset);

  if (base->methods && base->methods->seek)
    base->offset = base->methods->seek (env, base, new_offset);
  else
    base->offset = new_offset;

  return base->offset;
}

RemixCount
remix_tell (RemixEnv * env, RemixBase * base)
{
  if (!base) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }
  return base->offset;
}

int
remix_flush (RemixEnv * env, RemixBase * base)
{
  if (!base) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }
  if (!base->methods || !base->methods->flush) {
    remix_set_error (env, REMIX_ERROR_INVALID);
    return -1;
  }
  return _remix_flush (env, base);
}

RemixMetaText *
remix_get_meta_text (RemixEnv * env, RemixBase * base)
{
  RemixPlugin * plugin = base->plugin;
  return (plugin ? plugin->metatext : NULL);
}

RemixMetaText *
remix_set_meta_text (RemixEnv * env, RemixBase * base, RemixMetaText * mt)
{
  RemixPlugin * plugin;
  RemixMetaText * old;

  if (base == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return RemixNone;
  }

  plugin = base->plugin;
  if (plugin == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return RemixNone;
  }

  old = plugin->metatext;
  plugin->metatext = mt;
  return old;
}

int
remix_is_writeable (RemixEnv * env, RemixBase * base)
{
  RemixPlugin * plugin;

  if (base == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  plugin = base->plugin;
  if (plugin == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  return (plugin->flags & REMIX_PLUGIN_WRITEABLE);
}

int
remix_is_seekable (RemixEnv * env, RemixBase * base)
{
  RemixPlugin * plugin;

  if (base == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  plugin = base->plugin;
  if (plugin == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  return (plugin->flags & REMIX_PLUGIN_SEEKABLE);
}

int
remix_is_cacheable (RemixEnv * env, RemixBase * base)
{
  RemixPlugin * plugin;

  if (base == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  plugin = base->plugin;
  if (plugin == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  return (plugin->flags & REMIX_PLUGIN_CACHEABLE);
}

int
remix_is_causal (RemixEnv * env, RemixBase * base)
{
  RemixPlugin * plugin;

  if (base == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  plugin = base->plugin;
  if (plugin == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  return (plugin->flags & REMIX_PLUGIN_CAUSAL);
}
