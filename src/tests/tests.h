
#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#else
#  define PRId64 "I64d"
#endif

/* FIXME: on Mac OS X, off_t is 64-bits.  Obviously we want a nicer
 * way to do it than this, but a quick fix is a good fix */
#ifdef __APPLE__
#  define PRI_off_t "q"
#else
#  define PRI_off_t "l"
#endif

#define INFO(str) \
  { printf ("----  %s ...\n", (str)); }

#define WARN(str) \
  { printf ("%s:%d: warning: %s\n", __FILE__, __LINE__, (str)); }

#define FAIL(str) \
  { printf ("%s:%d: %s\n", __FILE__, __LINE__, (str)); exit(1); }

#undef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
