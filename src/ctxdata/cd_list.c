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
 * CDList: A generic doubly-linked list implementation.
 *
 * Conrad Parker <Conrad.Parker@CSIRO.AU>, August 2001
 */


#include "ctxdata.h"

static CDList *
cd_list_item_new (void * ctx, CDScalar data)
{
  CDList * l;

  l = cd_malloc (sizeof (struct _CDList));
  l->prev = l->next = NULL;
  l->data = data;

  return l;
}

CDList *
cd_list_new (void * ctx)
{
  return NULL;
}

CDList *
cd_list_copy (void * ctx, CDList * list)
{
  CDList * new_list = cd_list_new (ctx);
  CDList * l;

  for (l = list; l; l = l->next) {
    new_list = cd_list_append (ctx, new_list, l->data);
  }

  return new_list;
}

CDList *
cd_list_clone (void * ctx, CDList * list, CDCloneFunc clone)
{
  CDList * new_list = cd_list_new (ctx);
  CDList * l;
  CDScalar new_data;

  for (l = list; l; l = l->next) {
    new_data = CD_POINTER (clone (ctx, l->data.s_pointer));
    new_list = cd_list_append (ctx, new_list, new_data);
  }

  return new_list;
}

CDList *
cd_list_clone_with (void * ctx, CDList * list, CDCloneWithFunc clone,
		    void * with)
{
  CDList * new_list = cd_list_new (ctx);
  CDList * l;
  CDScalar new_data;

  for (l = list; l; l = l->next) {
    new_data = CD_POINTER (clone (ctx, l->data.s_pointer, with));
    new_list = cd_list_append (ctx, new_list, new_data);
  }

  return new_list;
}

CDList *
cd_list_last_item (void * ctx, CDList * list)
{
  CDList * l;
  for (l = list; l; l = l->next)
    if (l->next == NULL) return l;
  return NULL;
}

CDScalar
cd_list_last (void * ctx, CDList * list, CDScalarType type)
{
  CDList * l = cd_list_last_item (ctx, list);
  return ((l == NULL) ? cd_scalar_invalid (ctx, type) : l->data);
}

CDList *
cd_list_prepend (void * ctx, CDList * list, CDScalar data)
{
  CDList * l = cd_list_item_new (ctx, data);

  if (list == NULL) return l;

  l->next = list;
  list->prev = l;

  return l;
}

CDList *
cd_list_append (void * ctx, CDList * list, CDScalar data)
{
  CDList * l = cd_list_item_new (ctx, data);
  CDList * last;

  if (list == NULL) return l;

  last = cd_list_last_item (ctx, list);
  if (last) last->next = l;
  l->prev = last; 
  return list;
}

CDList *
cd_list_add_before_item (void * ctx, CDList * list, CDScalar data,
			 CDList * item)
{
  CDList * l, * p;

  if (list == NULL) return cd_list_item_new (ctx, data);
  if (item == NULL) return cd_list_append (ctx, list, data);
  if (item == list) return cd_list_prepend (ctx, list, data);

  l = cd_list_item_new (ctx, data);
  p = item->prev;

  l->prev = p;
  l->next = item;
  if (p) p->next = l;
  item->prev = l;
  
  return list;
}

CDList *
cd_list_add_after_item (void * ctx, CDList * list, CDScalar data,
			CDList * item)
{
  CDList * l, * n;

  if (item == NULL) return cd_list_prepend (ctx, list, data);

  l = cd_list_item_new (ctx, data);
  n = item->next;

  l->prev = item;
  l->next = n;
  if (n) n->prev = l;
  item->next = l;

  return list;
}

CDList *
cd_list_add_before (void * ctx, CDList * list, CDScalarType type,
		    CDScalar data, CDScalar before)
{
  CDList * b = cd_list_find (ctx, list, type, before);
  return cd_list_add_before_item (ctx, list, data, b);
}

CDList *
cd_list_add_after (void * ctx, CDList * list, CDScalarType type,
		   CDScalar data, CDScalar after)
{
  CDList * a = cd_list_find (ctx, list, type, after);
  return cd_list_add_after_item (ctx, list, data, a);
}


CDList *
cd_list_find (void * ctx, CDList * list, CDScalarType type, CDScalar data)
{
  CDList * l;

  for (l = list; l; l = l->next)
    if (cd_scalar_eq (ctx, type, l->data, data)) return l;

  return NULL;
}

/*
 * cd_list_find_first (ctx, list, f, data)
 *
 * Finds the first item l of 'list' for which f (ctx, l->data, data) is true.
 * Returns NULL if the condition is not true for any item of 'list'.
 */
CDList *
cd_list_find_first (void * ctx, CDList * list, CDScalarType type,
		    CDCmpFunc f, CDScalar data)
{
  CDList * l;

  switch (type) {
  case CD_TYPE_CHAR:
    for (l = list; l; l = l->next)
      if (((CDCmpCharFunc)f)(ctx, l->data.s_char, data.s_char)) return l;
    break;
  case CD_TYPE_UCHAR:
    for (l = list; l; l = l->next)
      if (((CDCmpUCharFunc)f)(ctx, l->data.s_uchar, data.s_uchar)) return l;
    break;
  case CD_TYPE_INT:
    for (l = list; l; l = l->next)
      if (((CDCmpIntFunc)f)(ctx, l->data.s_int, data.s_int)) return l;
    break;
  case CD_TYPE_UINT:
    for (l = list; l; l = l->next)
      if (((CDCmpUIntFunc)f)(ctx, l->data.s_uint, data.s_uint)) return l;
    break;
  case CD_TYPE_LONG:
    for (l = list; l; l = l->next)
      if (((CDCmpLongFunc)f)(ctx, l->data.s_long, data.s_long)) return l;
    break;
  case CD_TYPE_ULONG:
    for (l = list; l; l = l->next)
      if (((CDCmpULongFunc)f)(ctx, l->data.s_ulong, data.s_ulong)) return l;
    break;
  case CD_TYPE_FLOAT:
    for (l = list; l; l = l->next)
      if (((CDCmpFloatFunc)f)(ctx, l->data.s_float, data.s_float)) return l;
    break;
  case CD_TYPE_DOUBLE:
    for (l = list; l; l = l->next)
      if (((CDCmpDoubleFunc)f)(ctx, l->data.s_double, data.s_double)) return l;
    break;
  case CD_TYPE_STRING:
    for (l = list; l; l = l->next)
      if (((CDCmpStringFunc)f)(ctx, l->data.s_string, data.s_string)) return l;
    break;
  case CD_TYPE_POINTER:
    for (l = list; l; l = l->next)
      if (((CDCmpFunc)f)(ctx, l->data.s_pointer, data.s_pointer)) return l;
    break;
  default: break;
  }

  return NULL;
}

/*
 * cd_list_find_last (ctx, list, f, data)
 *
 * Finds the last item l of 'list' for which f (ctx, l->data, data) is true.
 * Returns NULL if no items are found for which the condition is true
 * before an item is found for which it is false.
 */
CDList *
cd_list_find_last (void * ctx, CDList * list, CDScalarType type,
		   CDCmpFunc f, CDScalar data)
{
  CDList * l, * lp = NULL;

  switch (type) {
  case CD_TYPE_CHAR:
    for (l = list; l; l = l->next) {
      if (!((CDCmpCharFunc)f)(ctx, l->data.s_char, data.s_char))
	return lp;
      lp = l;
    }
    break;
  case CD_TYPE_UCHAR:
    for (l = list; l; l = l->next) {
      if (!((CDCmpUCharFunc)f)(ctx, l->data.s_uchar, data.s_uchar))
	return lp;
      lp = l;
    }
    break;
  case CD_TYPE_INT:
    for (l = list; l; l = l->next) {
      if (!((CDCmpIntFunc)f)(ctx, l->data.s_int, data.s_int))
	return lp;
      lp = l;
    }
    break;
  case CD_TYPE_UINT:
    for (l = list; l; l = l->next) {
      if (!((CDCmpUIntFunc)f)(ctx, l->data.s_uint, data.s_uint))
	return lp;
      lp = l;
    }
    break;
  case CD_TYPE_LONG:
    for (l = list; l; l = l->next) {
      if (!((CDCmpLongFunc)f)(ctx, l->data.s_long, data.s_long))
	return lp;
      lp = l;
    }
    break;
  case CD_TYPE_ULONG:
    for (l = list; l; l = l->next) {
      if (!((CDCmpULongFunc)f)(ctx, l->data.s_ulong, data.s_ulong))
	return lp;
      lp = l;
    }
    break;
  case CD_TYPE_FLOAT:
    for (l = list; l; l = l->next) {
      if (!((CDCmpFloatFunc)f)(ctx, l->data.s_float, data.s_float))
	return lp;
      lp = l;
    }
    break;
  case CD_TYPE_DOUBLE:
    for (l = list; l; l = l->next) {
      if (!((CDCmpDoubleFunc)f)(ctx, l->data.s_double, data.s_double))
	return lp;
      lp = l;
    }
    break;
  case CD_TYPE_STRING:
    for (l = list; l; l = l->next) {
      if (!((CDCmpStringFunc)f)(ctx, l->data.s_string, data.s_string))
	return lp;
      lp = l;
    }
    break;
  case CD_TYPE_POINTER:
    for (l = list; l; l = l->next) {
      if (!((CDCmpFunc)f)(ctx, l->data.s_pointer, data.s_pointer))
	return lp;
      lp = l;
    }
    break;
  default: break;
  }

  return lp;
}

/*
 * cd_list_insert (ctx, list, data, f)
 *
 * Inserts data into sorted list 'list' using the comparison function 'f'.
 * 'data' is inserted before the first item l found for which f(ctx, l, data)
 * is true.
 */
CDList *
cd_list_insert (void * ctx, CDList * list, CDScalarType type, CDScalar data,
		CDCmpFunc f)
{
  CDList * l;
  if (list == NULL) return cd_list_prepend (ctx, list, data);
  if ((l = cd_list_find_first (ctx, list, type, f, data)) == NULL)
    return cd_list_append (ctx, list, data);
  return cd_list_add_before_item (ctx, list, data, l);
}

CDList *
cd_list_remove (void * ctx, CDList * list, CDScalarType type, CDScalar data)
{
  CDList * l = cd_list_find (ctx, list, type, data);

  if (l == NULL) return list;

  if (l->prev) l->prev->next = l->next;
  if (l->next) l->next->prev = l->prev;

  if (l == list) return list->next;
  else return list;
}

CDList *
cd_list_join (void * ctx, CDList * l1, CDList * l2)
{
  CDList * l1_last = cd_list_last_item (ctx, l1);

  if (l1_last == NULL) return l2;

  l1_last->next = l2;
  if (l2 != NULL) l2->prev = l1_last;

  return l1;
}

int
cd_list_length (void * ctx, CDList * list)
{
  CDList * l;
  int c = 0;

  for (l = list; l; l = l->next)
    c++;

  return c;
}

int
cd_list_is_empty (void * ctx, CDList * list)
{
  return (list == NULL);
}

int
cd_list_is_singleton (void * ctx, CDList * list)
{
  if (list == NULL) return 0;
  if (list->next == NULL) return 1;
  else return 0;
}

/*
 * cd_list_destroy_with (ctx, list, destroy)
 *
 * Step through list 'list', destroying each item using AxDestroyFunc destroy,
 * and also free the list structure itself.
 */
CDList *
cd_list_destroy_with (void * ctx, CDList * list, CDDestroyFunc destroy)
{
  CDList * l, * ln;

  for (l = list; l; l = ln) {
    ln = l->next;
    destroy (ctx, l->data.s_pointer);
    cd_free (l);
  }

  return NULL;
}

/*
 * cd_list_free_all (ctx, list)
 *
 * Step through list 'list', freeing each item using cd_free(), and
 * also free the list structure itself.
 */
CDList *
cd_list_free_all (void * ctx, CDList * list)
{
  return cd_list_destroy_with (ctx, list, (CDDestroyFunc)cd_free);
}

/*
 * cd_list_free (list)
 *
 * Free the list structure 'list', but not its items.
 */
CDList *
cd_list_free (void * ctx, CDList * list)
{
  CDList * l, * ln;

  for (l = list; l; l = ln) {
    ln = l->next;
    cd_free (l);
  }

  return NULL;
}

void
cd_list_apply (void * ctx, CDList * list, CDFunc func)
{
  CDList * l;

  for (l = list; l; l = l->next) {
    func (ctx, l->data.s_pointer);
  }
}
