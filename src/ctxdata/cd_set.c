/*
 * CtxData -- Context oriented data types
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
 * CDSet: A keyed set implementation.
 *
 * Conrad Parker <Conrad.Parker@CSIRO.AU>, August 2001
 */


#include "ctxdata.h"

static CDSet *
cd_set_item_new (void * ctx, int key, CDScalar data)
{
  CDSet * s;

  s = cd_malloc (sizeof (struct _CDSet));
  s->prev = s->next = NULL;
  s->key = key;
  s->data = data;

  return s;
}

CDSet *
cd_set_new (void * ctx)
{
  return NULL;
}

CDSet *
cd_set_clone (void * ctx, CDSet * set, CDCloneFunc clone)
{
  CDSet * new_set = cd_set_new (ctx);
  CDSet * s;
  void * new_data;

  for (s = set; s; s = s->next) {
    new_data = clone (ctx, s->data.s_pointer);
    new_set = cd_set_insert (ctx, new_set, s->key, CD_POINTER(new_data));
  }

  return new_set;
}

CDSet *
cd_set_clone_keys (void * ctx, CDSet * set)
{
  CDSet * new_set = cd_set_new (ctx);
  CDSet * s;

  for (s = set; s; s = s->next) {
    new_set = cd_set_insert (ctx, new_set, s->key, CD_INT(0));
  }

  return new_set;
}

CDSet *
cd_set_clone_with (void * ctx, CDSet * set, CDCloneWithFunc clone,
		   void * with)
{
  CDSet * new_set = cd_set_new (ctx);
  CDSet * s;
  void * new_data;

  for (s = set; s; s = s->next) {
    new_data = clone (ctx, s->data.s_pointer, with);
    new_set = cd_set_insert (ctx, new_set, s->key, CD_POINTER(new_data));
  }

  return new_set;
}

static CDSet *
cd_set_find_item (void * ctx, CDSet * set, int key)
{
  CDSet * s;

  for (s = set; s; s = s->next)
    if (s->key == key) return s;

  return NULL;
}

int
cd_set_contains (void * ctx, CDSet * set, int key)
{
  CDSet * s = cd_set_find_item (ctx, set, key);
  return (s != NULL);
}

CDScalar
cd_set_find (void * ctx, CDSet * set, int key)
{
  CDSet * s = cd_set_find_item (ctx, set, key);
  if (s == NULL) return CD_POINTER(NULL);
  return s->data;
}
  

/*
 * cd_set_insert (ctx, set, key, data)
 *
 * Inserts data into set 'set'.
 */
CDSet *
cd_set_insert (void * ctx, CDSet * set, int key, CDScalar data)
{
  CDSet * check, * s;

  check = cd_set_find_item (ctx, set, key);
  if (check != NULL) return NULL; /* already contained */

  s = cd_set_item_new (ctx, key, data);

  if (set == NULL) return s;

  s->next = set;
  set->prev = s;

  return s;
}

CDSet *
cd_set_remove (void * ctx, CDSet * set, int key)
{
  CDSet * s = cd_set_find_item (ctx, set, key);

  if (s == NULL) return set;

  if (s->prev) s->prev->next = s->next;
  if (s->next) s->next->prev = s->prev;

  if (s == set) return set->next;
  else return set;
}

CDSet *
cd_set_replace (void * ctx, CDSet * set, int key, CDScalar data)
{
  CDSet * s = cd_set_find_item (ctx, set, key);

  if (s == NULL) { /* Not previously contained */
    return cd_set_insert (ctx, set, key, data);
  }

  s->data = data;

  return set;
}

int
cd_set_size (void * ctx, CDSet * set)
{
  CDSet * s;
  int n = 0;

  for (s = set; s; s = s->next)
    n++;

  return n;
}

int
cd_set_is_empty (void * ctx, CDSet * set)
{
  return (set == NULL);
}

int
cd_set_is_singleton (void * ctx, CDSet * set)
{
  if (set == NULL) return 0;
  if (set->next == NULL) return 1;
  else return 0;
}

/*
 * cd_set_destroy_with (ctx, set, destroy)
 *
 * Step through set 'set', destroying each item using CDDestroyFunc destroy,
 * and also free the set structure itself.
 */
CDSet *
cd_set_destroy_with (void * ctx, CDSet * set, CDDestroyFunc destroy)
{
  CDSet * s, * sn;

  for (s = set; s; s = sn) {
    sn = s->next;
    destroy (ctx, s->data.s_pointer);
    cd_free (s);
  }

  return NULL;
}

/*
 * cd_set_free_all (ctx, set)
 *
 * Step through set 'set', freeing each item using cd_free(), and
 * also free the set structure itself.
 */
CDSet *
cd_set_free_all (void * ctx, CDSet * set)
{
  return cd_set_destroy_with (ctx, set, (CDDestroyFunc)cd_free);
}

/*
 * cd_set_free (set)
 *
 * Free the set structure 'set', but not its items.
 */
CDSet *
cd_set_free (void * ctx, CDSet * set)
{
  CDSet * s, * sn;

  for (s = set; s; s = sn) {
    sn = s->next;
    cd_free (s);
  }

  return NULL;
}
