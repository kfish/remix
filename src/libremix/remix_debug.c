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
 * remix_debug.c -- Debuggin routines for Remix
 *
 * Conrad Parker <Conrad.Parker@CSIRO.AU>, August 2001
 *
 * Description
 * -----------
 *
 * This file contains printing routines for formatted debugging.
 *
 */

#include <string.h>
#include <stdarg.h>

#define __REMIX__
#include "remix.h"

static int indent = 0;

/*
 * remix_debug_down (void)
 */
void
remix_debug_down (void)
{
  indent ++;
}

/*
 * remix_debug_up (void)
 */
void
remix_debug_up (void)
{
  indent --;
}

/*
 * remix_dprintf (fmt)
 *
 * Print a formatted debugging message to stdout at the current indent
 */
void
remix_dprintf (const char * fmt, ...)
{
#ifdef DEBUG
  va_list ap;
  char buf[4096];
  int i, n;

  va_start (ap, fmt);

  n = MIN (4096, indent);

  for (i = 0; i < n; i++) {
    buf[i] = ' ';
  }

  vsnprintf (buf+n, 4096-n, fmt, ap);

  fputs (buf, stdout);
  fflush (NULL);

  va_end (ap);
#endif
}
