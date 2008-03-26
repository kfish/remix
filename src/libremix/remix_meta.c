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
 * RemixMetaText: Metadata for RemixBases.
 *
 * Conrad Parker <conrad@metadecks.org>, August 2001
 */


#define __REMIX__
#include "remix.h"

#include <string.h>

RemixMetaText *
remix_meta_text_new (RemixEnv * env)
{
  RemixMetaText * mt;

  mt = (RemixMetaText *) remix_malloc (sizeof (struct _RemixMetaText));
  mt->authors = cd_list_new (env);

  return mt;
}

char *
remix_meta_text_get_identifier (RemixEnv * env, RemixMetaText * mt)
{
  return mt->identifier;
}

char *
remix_meta_text_set_identifier (RemixEnv * env, RemixMetaText * mt, char * identifier)
{
  char * old = mt->identifier;
  mt->identifier = (identifier == NULL) ? NULL : strdup(identifier);
  return old;
}

char *
remix_meta_text_get_category (RemixEnv * env, RemixMetaText * mt)
{
  return mt->category;
}

char *
remix_meta_text_set_category (RemixEnv * env, RemixMetaText * mt, char * category)
{
  char * old = mt->category;
  mt->category = (category == NULL) ? NULL : strdup(category);
  return old;
}

char *
remix_meta_text_get_description (RemixEnv * env, RemixMetaText * mt)
{
  return mt->description;
}

char *
remix_meta_text_set_description (RemixEnv * env, RemixMetaText * mt, char * description)
{
  char * old = mt->description;
  mt->description = (description == NULL) ? NULL : strdup(description);
  return old;
}

char *
remix_meta_text_get_copyright (RemixEnv * env, RemixMetaText * mt)
{
  return mt->copyright;
}

char *
remix_meta_text_set_copyright (RemixEnv * env, RemixMetaText * mt, char * copyright)
{
  char * old = mt->copyright;
  mt->copyright = (copyright == NULL) ? NULL : strdup(copyright);
  return old;
}

char *
remix_meta_text_get_url (RemixEnv * env, RemixMetaText * mt)
{
  return mt->url;
}

char *
remix_meta_text_set_url (RemixEnv * env, RemixMetaText * mt, char * url)
{
  char * old = mt->url;
  mt->url = (url == NULL) ? NULL : strdup(url);
  return old;
}

static RemixMetaAuthor *
remix_meta_author_new (RemixEnv * env, char * name, char * email)
{
  RemixMetaAuthor * ma = remix_malloc (sizeof (struct _RemixMetaAuthor));
  ma->name = (name == NULL) ? NULL : strdup(name);
  ma->email = (email == NULL) ? NULL: strdup(email);
  return ma;
}

CDList *
remix_meta_text_get_authors (RemixEnv * env, RemixMetaText * mt)
{
  return mt->authors;
}
 
void
remix_meta_text_add_author (RemixEnv * env, RemixMetaText * mt,
			 char * name, char * email)
{
  RemixMetaAuthor * ma = remix_meta_author_new (env, name, email);
  mt->authors = cd_list_append (env, mt->authors, CD_POINTER(ma));
}

void
remix_meta_text_free (RemixEnv * env, RemixMetaText * mt)
{
  cd_list_free_all (env, mt->authors);
  remix_free (mt);
}
