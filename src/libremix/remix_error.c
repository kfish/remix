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
 * RemixError -- SOUNDRENDER error handling.
 *
 * Conrad Parker <Conrad.Parker@CSIRO.AU>, August 2001
 */

#define __REMIX__
#include "remix.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

char *
remix_error_string (RemixEnv * env, RemixError error)
{
  switch (error) {
  case REMIX_ERROR_OK: return "OK"; break;
  case REMIX_ERROR_INVALID: return "Invalid base"; break;
  case REMIX_ERROR_NOENTITY: return "No such base"; break;
  case REMIX_ERROR_SILENCE: return "Operation would yield silence"; break;
  case REMIX_ERROR_NOOP: return "Operation would not modify data"; break;
  case REMIX_ERROR_SYSTEM: return "System error"; break;
  default: return "Unknown error"; break;
  }
}

/*
 * remix_exit_err (fmt)
 *
 * Print a formatted error message and errno information to stderr,
 * then exit with return code 1.
 */
void
remix_exit_err (const char * fmt, ...)
{
  va_list ap;
  int errno_save;
  char buf[REMIX_MAXLINE];
  int n;

  errno_save = errno;

  va_start (ap, fmt);

  snprintf (buf, REMIX_MAXLINE, "SOUNDRENDER: ");
  n = strlen (buf);

  vsnprintf (buf+n, REMIX_MAXLINE-n, fmt, ap);
  n = strlen (buf);

  snprintf (buf+n, REMIX_MAXLINE-n, ": %s\n", strerror (errno_save));

  fflush (stdout); /* in case stdout and stderr are the same */
  fputs (buf, stderr);
  fflush (NULL);

  va_end (ap);
  exit (1);
}

/*
 * remix_print_err (fmt)
 *
 * Print a formatted error message to stderr.
 */
void
remix_print_err (const char * fmt, ...)
{
  va_list ap;
  int errno_save;
  char buf[REMIX_MAXLINE];
  int n;

  errno_save = errno;

  va_start (ap, fmt);

  snprintf (buf, REMIX_MAXLINE, "SOUNDRENDER: ");
  n = strlen (buf);

  vsnprintf (buf+n, REMIX_MAXLINE-n, fmt, ap);
  n = strlen (buf);

  fflush (stdout); /* in case stdout and stderr are the same */
  fputs (buf, stderr);
  fputc ('\n', stderr);
  fflush (NULL);

  va_end (ap);
}

/*
 * print_debug (level, fmt)
 *
 * Print a formatted debugging message of level 'level' to stderr
 */

