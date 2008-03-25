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
 * RemixNull: A RemixBase with no processing
 *
 * Conrad Parker <Conrad.Parker@CSIRO.AU>, August 2001
 *
 * Description
 * -----------
 *
 * This is a small set of base functions (process, length and seek)
 * which are useful as optimised versions of an empty container base;
 * see eg. RemixDeck.
 * The process function always returns with error REMIX_ERROR_NOOP.
 * The length function always returns 0
 * The seek function always returns its argument. Note the seek function
 * is not needed; you may define an base's seek function as NULL for
 * the same effect.
 *
 * Invariants
 * ----------
 *
 * There is nothing to be invariant about here :)
 *
 */

#define __REMIX__
#include "remix.h"

RemixCount
remix_null_length (RemixEnv * env, RemixBase * base)
{
  return 0;
}

RemixCount
remix_null_process (RemixEnv * env, RemixBase * base, RemixCount count,
                    RemixStream * input, RemixStream * output)
{
  remix_set_error (env, REMIX_ERROR_NOOP);
  return 0;
}

RemixCount
remix_null_seek (RemixEnv * env, RemixBase * base, RemixCount offset)
{
  return offset;
}
