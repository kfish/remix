/*
 * CtxData -- Context dependent data types
 *
 * Copyright (C) 2001 Conrad Parker <conrad@vergenet.net>
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

#ifndef __CTXDATA_H__
#define __CTXDATA_H__

#include <stdlib.h>

#define cd_malloc malloc
#define cd_free free

#ifdef FALSE
#undef FALSE
#endif
#define FALSE (0)

#ifdef TRUE
#undef TRUE
#endif
#define TRUE (!FALSE)

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

typedef enum _CDScalarType CDScalarType;
typedef union _CDScalar CDScalar;
typedef struct _CDSet CDSet;
typedef struct _CDList CDList;

#define CD_ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

typedef void * (*CDOpaqueFunc) (void);

typedef void * (*CDFunc) (void * ctx, void * data);
typedef void * (*CDCloneFunc) (void * ctx, void * data);
typedef void * (*CDCloneWithFunc) (void * ctx, void * data, void * with);
typedef void (*CDFreeFunc) (void * ctx, void * data);
typedef int (*CDDestroyFunc) (void * ctx, void * data);

typedef int (*CDCmpBoolFunc) (void * ctx, int data1, int data2);
typedef int (*CDCmpCharFunc) (void * ctx, char data1, char data2);
typedef int (*CDCmpUCharFunc) (void * ctx, unsigned char data1,
			       unsigned char data2);
typedef int (*CDCmpIntFunc) (void * ctx, int data1, int data2);
typedef int (*CDCmpUIntFunc) (void * ctx, unsigned int data1,
			      unsigned int data2);
typedef int (*CDCmpLongFunc) (void * ctx, long data1, long data2);
typedef int (*CDCmpULongFunc) (void * ctx, unsigned long data1,
			       unsigned long data2);
typedef int (*CDCmpFloatFunc) (void * ctx, float data1, float data2);
typedef int (*CDCmpDoubleFunc) (void * ctx, double data1, double data2);
typedef int (*CDCmpStringFunc) (void * ctx, char * data1, char * data2);

typedef int (*CDCmpFunc) (void * ctx, void * data1, void * data2);

enum _CDScalarType {
  CD_TYPE_CHAR,
  CD_TYPE_UCHAR,
  CD_TYPE_INT,
  CD_TYPE_UINT,
  CD_TYPE_LONG,
  CD_TYPE_ULONG,
  CD_TYPE_FLOAT,
  CD_TYPE_DOUBLE,
  CD_TYPE_STRING,
  CD_TYPE_POINTER
};

union _CDScalar {
  int s_bool;
  char s_char;
  unsigned char s_uchar;
  int s_int;
  unsigned int s_uint;
  long s_long;
  unsigned long s_ulong;
  float s_float;
  double s_double;
  char * s_string;
  void * s_pointer;
};

#define CD_BOOL(x) ((CDScalar)((unsigned char)(x)))
#define CD_CHAR(x) ((CDScalar)((char)(x)))
#define CD_UCHAR(x) ((CDScalar)((unsigned char)(x)))
#define CD_INT(x) ((CDScalar)((int)(x)))
#define CD_UINT(x) ((CDScalar)((unsigned int)(x)))
#define CD_LONG(x) ((CDScalar)((long)(x)))
#define CD_ULONG(x) ((CDScalar)((unsigned long)(x)))
#define CD_FLOAT(x) ((CDScalar)((float)(x)))
#define CD_DOUBLE(x) ((CDScalar)((double)(x)))
#define CD_STRING(x) ((CDScalar)((char *)(x)))
#define CD_POINTER(x) ((CDScalar)((void *)(x)))

struct _CDSet {
  CDSet * prev;
  CDSet * next;
  int key;
  CDScalar data;
};

struct _CDList {
  CDList * prev;
  CDList * next;
  CDScalar data;
};

/* cd_type */

CDScalar cd_scalar_invalid (void * ctx, CDScalarType type);
int cd_scalar_eq (void * ctx, CDScalarType type, CDScalar d1, CDScalar d2);

/* cd_set */

#define CD_EMPTY_SET NULL
#define CD_SINGLETON_SET(k,x) (&(((struct _CDSet){NULL, NULL, (k), (x)})))

CDSet * cd_set_new (void * ctx);
CDSet * cd_set_clone (void * ctx, CDSet * set, CDCloneFunc clone);
CDSet * cd_set_clone_keys (void * ctx, CDSet * set);
CDSet * cd_set_clone_with (void * ctx, CDSet * set, CDCloneWithFunc clone,
			   void * with);
int cd_set_contains (void * ctx, CDSet * set, int key);
CDScalar cd_set_find (void * ctx, CDSet * set, int key);
CDSet * cd_set_insert (void * ctx, CDSet * set, int key, CDScalar data);
CDSet * cd_set_remove (void * ctx, CDSet * set, int key);
CDSet * cd_set_replace (void * ctx, CDSet * set, int key, CDScalar data);
int cd_set_is_empty (void * ctx, CDSet * set);
int cd_set_is_singleton (void * ctx, CDSet * set);
int cd_set_size (void * ctx, CDSet * set);
CDSet * cd_set_destroy_with (void * ctx, CDSet * set, CDDestroyFunc destroy);
CDSet * cd_set_free_all (void * ctx, CDSet * set);
CDSet * cd_set_free (void * ctx, CDSet * set);


/* cd_list */

#define CD_EMPTY_LIST NULL
#define CD_SINGLETON_LIST(x) (&(((struct _CDList){NULL, NULL, (x)})))

CDList * cd_list_new (void * ctx);
CDList * cd_list_copy (void * ctx, CDList * list);
CDList * cd_list_clone (void * ctx, CDList * list, CDCloneFunc clone);
CDList * cd_list_clone_with (void * ctx, CDList * list, CDCloneWithFunc clone,
			     void * with);
CDList * cd_list_last_item (void * ctx, CDList * list);
CDScalar cd_list_last (void * ctx, CDList * list, CDScalarType type);
CDList * cd_list_prepend (void * ctx, CDList * list, CDScalar data);
CDList * cd_list_add_before_item (void * ctx, CDList * list, CDScalar data,
				  CDList * item);
CDList * cd_list_add_after_item (void * ctx, CDList * list, CDScalar data,
				 CDList * item);
CDList * cd_list_add_before (void * ctx, CDList * list, CDScalarType type,
			     CDScalar data, CDScalar before);
CDList * cd_list_add_after (void * ctx, CDList * list, CDScalarType type,
			    CDScalar data, CDScalar after);
CDList * cd_list_append (void * ctx, CDList * list, CDScalar data);
CDList * cd_list_find (void * ctx, CDList * list, CDScalarType type,
		       CDScalar data);
CDList * cd_list_find_first (void * ctx, CDList * list, CDScalarType type,
			     CDCmpFunc f, CDScalar data);
CDList * cd_list_find_last (void * ctx, CDList * list, CDScalarType type,
			    CDCmpFunc f, CDScalar data);
CDList * cd_list_insert (void * ctx, CDList * list, CDScalarType type,
			 CDScalar data, CDCmpFunc f);
CDList * cd_list_remove (void * ctx, CDList * list, CDScalarType type,
			 CDScalar data);
CDList * cd_list_join (void * ctx, CDList * l1, CDList * l2);
int cd_list_length (void * ctx, CDList * list);
int cd_list_is_empty (void * ctx, CDList * list);
int cd_list_is_singleton (void * ctx, CDList * list);
CDList * cd_list_destroy_with (void * ctx, CDList * list,
			       CDDestroyFunc destroy);
CDList * cd_list_free_all (void * ctx, CDList * list);
CDList * cd_list_free (void * ctx, CDList * list);

void cd_list_apply (void * ctx, CDList * list, CDFunc func);

#endif /* __CTXDATA_H__ */
