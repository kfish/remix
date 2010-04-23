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
 * RemixPlugin: A container for RemixBase types.
 *
 * Conrad Parker <conrad@metadecks.org>, August 2001
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/stat.h>

#define __REMIX__
#include "remix.h"

static CDList * modules_list = CD_EMPTY_LIST;

static CDList *
remix_plugin_initialise_static (RemixEnv * env)
{
  CDList * plugins = cd_list_new (env);

  plugins = cd_list_join (env, plugins, __gain_init (env));

#ifdef HAVE_LIBSNDFILE1
  plugins = cd_list_join (env, plugins, __sndfile_init (env));
#endif

  return plugins;
}

static CDList *
remix_plugin_init (RemixEnv * env, const char * path)
{
  void * module;
  CDList * l;
  RemixPluginInitFunc init;

  module = dlopen (path, RTLD_NOW);

  if (!module) {
    remix_dprintf ("[remix_plugin_init] Unable to open %s: %s\n", path,
		   dlerror ());
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return CD_EMPTY_LIST;
  }

  /* Check that this module has not already been loaded (eg. if it is
   * a symlink etc.) */
  for (l = modules_list; l; l = l->next) {
    if (l->data.s_pointer == module) {
      dlclose (module);
      return CD_EMPTY_LIST;
    }
  }

  modules_list = cd_list_append (env, modules_list, CD_POINTER(module));

  if ((init = dlsym (module, "remix_load")) != NULL) {
    return init (env);
  }

  return CD_EMPTY_LIST;
}

#define BUFLEN 256

static CDList *
init_dynamic_plugins_dir (RemixEnv * env, char * dirname)
{
  CDList * plugins = cd_list_new (env);
  DIR * dir;
  struct dirent * dirent;
  char * name;
  static char buf[BUFLEN];
  struct stat statbuf;

  dir = opendir (dirname);
  if (!dir) {
    /* fail silently */
    return CD_EMPTY_LIST;
  }

  while ((dirent = readdir (dir)) != NULL) {
    name = dirent->d_name;

    remix_dprintf ("[init_dynamic_plugins_dir] trying %s ... ", name);
    snprintf (buf, BUFLEN, "%s/%s", dirname, name);
      
    if (stat (buf, &statbuf) == -1) {
      remix_set_error (env, REMIX_ERROR_SYSTEM);
    } else if (remix_stat_regular (statbuf.st_mode)) {
      plugins = cd_list_join (env, plugins, remix_plugin_init (env, buf));
    }
  }

  closedir (dir);

  return plugins;
}

static CDList *
remix_plugin_initialise_dynamic (RemixEnv * env)
{
  return init_dynamic_plugins_dir (env, PACKAGE_PLUGIN_DIR);
}

void
remix_plugin_defaults_initialise (RemixEnv * env)
{
  CDList * plugins = cd_list_new (env);

  plugins = cd_list_join (env, plugins, remix_plugin_initialise_static (env));
  plugins = cd_list_join (env, plugins, remix_plugin_initialise_dynamic (env));

  cd_list_apply (env, plugins, (CDFunc)_remix_register_plugin);

  cd_list_free (env, plugins);
}

static int
remix_plugin_unload (RemixEnv * env, void * module)
{
  RemixPluginInitFunc unload; /* TODO: make new unload type */

  if ((unload = dlsym (module, "remix_unload")) != NULL) {
    unload (env);
  }

  dlclose (module);

  return 0;
}

void
remix_plugin_defaults_unload (RemixEnv * env)
{
  CDList * l;

  modules_list = cd_list_destroy_with (env, modules_list, (CDDestroyFunc)remix_plugin_unload);
}

#if 0

RemixPlugin
remix_plugin_new (void)
{
  RemixPlugin plugin = remix_malloc (sizeof (struct _RemixPlugin));
#if 0
  plugin->text = remix_meta_text_new ();
#endif
  return plugin;
}

RemixMetaText *
remix_plugin_get_meta_text (RemixPlugin * plugin)
{
  return plugin->text;
}

RemixMetaText *
remix_plugin_set_meta_text (RemixPlugin * plugin, RemixMetaText * mt)
{
  RemixMetaText * old = plugin->text;
  plugin->text = mt;
  return old;
}

int
remix_plugin_writeable (RemixPlugin * plugin)
{
  return (plugin->flags & REMIX_PLUGIN_WRITEABLE);
}

int
remix_plugin_seekable (RemixPlugin * plugin)
{
  return (plugin->flags & REMIX_PLUGIN_SEEKABLE);
}

int
remix_plugin_cacheable (RemixPlugin * plugin)
{
  return (plugin->flags & REMIX_PLUGIN_CACHEABLE);
}

int
remix_plugin_causal (RemixPlugin * plugin)
{
  return (plugin->flags & REMIX_PLUGIN_CAUSAL);
}

#endif
