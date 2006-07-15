#include <stdio.h>

#include <ctxdata.h>

void
ildump (void * ctx, CDList * list)
{
  CDList * l;
  int *i;

  if (list == NULL) {
    printf ("NULL");
  } else {
    for (l=list; l; l = l->next) {
      i = (int *)l->data.s_pointer;
      printf ("%d ", *i);
    }
  }

  printf ("\n\t[length %d, %s, %s]\n", cd_list_length (ctx, list),
    cd_list_is_empty (ctx, list) ? "empty" : "non-empty",
    cd_list_is_singleton (ctx, list) ? "singleton" : "non-singleton");
}

int
int_gt (void * ctx, void * d1, void * d2)
{
  int i1 = *(int *)d1;
  int i2 = *(int *)d2;
  return (i1 > i2);
}

void *
int_clone (void * ctx, void * data)
{
  int * n = cd_malloc (sizeof (int));
  *n = *(int *)data;
  return (void *)n;
}

int
main (int argc, char ** argv)
{
  CDList * list, * list2;
  void * ctx;
  int a=1, b=2, d=4, f=6, g=7, k=11;
  int v=22, w=23;
  int z=26;

  ctx = NULL;

  printf ("creating new list\n");
  list = cd_list_new (ctx);
  ildump (ctx, list);

  printf ("appending %d\n", f);
  list = cd_list_append (ctx, list, (CDScalar)(void *)(&f));
  ildump (ctx, list);

  printf ("appending %d\n", k);
  list = cd_list_append (ctx, list, CD_POINTER(&k));
  ildump (ctx, list);

  printf ("inserting %d\n", g);
  list = cd_list_insert (ctx, list, CD_TYPE_POINTER, CD_POINTER(&g), int_gt);
  ildump (ctx, list);

  printf ("inserting %d\n", w);
  list = cd_list_insert (ctx, list, CD_TYPE_POINTER, CD_POINTER(&w), int_gt);
  ildump (ctx, list);

  printf ("inserting %d\n", z);
  list = cd_list_insert (ctx, list, CD_TYPE_POINTER, CD_POINTER(&z), int_gt);
  ildump (ctx, list);

  printf ("inserting %d\n", b);
  list = cd_list_insert (ctx, list, CD_TYPE_POINTER, CD_POINTER(&b), int_gt);
  ildump (ctx, list);

  printf ("prepending %d\n", a);
  list = cd_list_prepend (ctx, list, CD_POINTER(&a));
  ildump (ctx, list);

  printf ("removing %d\n", g);
  list = cd_list_remove (ctx, list, CD_TYPE_POINTER, CD_POINTER(&g));
  ildump (ctx, list);

  printf ("adding %d after %d\n", d, b);
  list = cd_list_add_after (ctx, list, CD_TYPE_POINTER, CD_POINTER(&d),
                            CD_POINTER(&b));
  ildump (ctx, list);

  printf ("adding %d before %d\n", v, w);
  list = cd_list_add_before (ctx, list, CD_TYPE_POINTER, CD_POINTER(&v),
                             CD_POINTER(&w));
  ildump (ctx, list);

  printf ("cloning list\n");
  list2 = cd_list_clone (ctx, list, int_clone);
  ildump (ctx, list2);

  printf ("freeing cloned list\n");
  list2 = cd_list_free_all (ctx, list2);
  ildump (ctx, list2);

  exit (0);
}
