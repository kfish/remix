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
 * RemixPCM: generic functions for anonymous blocks of PCM data.
 *
 * Conrad Parker <conrad@metadecks.org>, August 2001
 *
 * Description
 * -----------
 *
 * This file contains only generic functions to manipulate blocks of
 * RemixPCM data. The RemixPCM type is defined in <remix_types.h>,  usually as
 * a floating point value (float).
 *
 * The functions in this file are an excellent target for short vector
 * machine optimisations.
 *
 * Invariants
 * ----------
 *
 * N/A
 *
 */

#include <string.h>

#define __REMIX__
#include "remix.h"


/* PFunc */

/*
 * _remix_pcm_clear_region (data, count)
 */
RemixCount
_remix_pcm_clear_region (RemixPCM * data, RemixCount count, void * unused)
{
  memset (data, (RemixPCM)0, count * sizeof (RemixPCM));
  return count;
}


/* PVFunc */

RemixCount
_remix_pcm_set (RemixPCM * data, RemixPCM value, RemixCount count)
{
  RemixCount i;

  for (i = 0; i < count; i++) {
    *data++ = value;
  }

  return count;
}

RemixCount
_remix_pcm_gain (RemixPCM * data, RemixCount count, void * gain)
{
  RemixPCM _gain = *(RemixPCM *)gain;
  RemixCount i;

  for (i = 0; i < count; i++) {
    *data++ *= _gain;
  }

  return count;
}


/* PPFunc */

/*
 * _remix_pcm_copy (src, dest, count)
 *
 * Copy PCM data from src to dest.
 */
RemixCount
_remix_pcm_copy (RemixPCM * src, RemixPCM * dest, RemixCount count,
                 void * unused)
{
  memcpy (dest, src, count * sizeof (RemixPCM));
  return count;
}

/*
 * _remix_pcm_add (src, dest, count)
 *
 * Add PCM data from src to dest.
 */
RemixCount
_remix_pcm_add (RemixPCM * src, RemixPCM * dest, RemixCount count,
                void * unused)
{
  RemixCount i;

  for (i = 0; i < count; i++) {
    *dest++ += *src++;
  }

  return count;
}

/*
 * _remix_pcm_mult (src, dest, count)
 *
 * Multiply PCM data of dest by that in src.
 */
RemixCount
_remix_pcm_mult (RemixPCM * src, RemixPCM * dest, RemixCount count,
                 void * unused)
{
  RemixCount i;

  for (i = 0; i < count; i++) {
    *dest++ *= *src++;
  }

  return count;
}

/*
 * _remix_pcm_fade (src, dest, count)
 *
 * Fade PCM data of dest by that in src.
 */
RemixCount
_remix_pcm_fade (RemixPCM * src, RemixPCM * dest, RemixCount count,
                 void * unused)
{
  RemixCount i;

  for (i = 0; i < count; i++) {
    *dest++ *= (1.0 - *src++);
  }

  return count;
}

/*
 * _remix_pcm_interleave_2 (src1, src2, count, dest)
 *
 * Interleave data of src1 and src2, storing result in dest
 */
RemixCount
_remix_pcm_interleave_2 (RemixPCM * src1, RemixPCM * src2, RemixCount count,
                         void * data)
{
  RemixPCM * dest = (RemixPCM *)data;
  RemixCount i;

  for (i = 0; i < count; i++) {
    *dest++ = *src1++;
    *dest++ = *src2++;
  }

  return count;
}

/*
 * _remix_pcm_deinterleave_2 (dest1, dest2, count, src)
 *
 * Deinterleave data of src, storing result in dest1 and dest2
 */
RemixCount
_remix_pcm_deinterleave_2 (RemixPCM * dest1, RemixPCM * dest2,
                           RemixCount count, void * data)
{
  RemixPCM * src = (RemixPCM *)data;
  RemixCount i;

  for (i = 0; i < count; i++) {
    *dest1++ = *src++;
    *dest2++ = *src++;
  }

  return count;
}

/* PPPFunc */

/*
 * _remix_pcm_blend (src, blend, dest, count)
 *
 * Blend PCM data of 'src' into 'dest' by blend values in 'blend'.
 */
RemixCount
_remix_pcm_blend (RemixPCM * src, RemixPCM * blend, RemixPCM * dest,
                  RemixCount count, void * unused)
{
  RemixCount i;
  RemixPCM b, d;

  for (i = 0; i < count; i++) {
    b = *blend++;
    d = (*dest * b) + (*src++ * (1.0 - b));
    *dest++ = d;
  }

  return count;
}


/* Miscellaneous */

/*
 * remix_pcm_write_linear (data, x1, y1, x2, y2, offset, count)
 *
 * Write 'count' samples at 'data' following line passing through (x1, y1)
 * and (x2, y2), with writing starting at x = 'offset'.
 */
RemixCount
_remix_pcm_write_linear (RemixPCM * data, RemixCount x1, RemixPCM y1,
                         RemixCount x2, RemixPCM y2,
                         RemixCount offset, RemixCount count)
{
  RemixCount i;

  remix_dprintf ("[remix_pcm_write_linear] ((%ld, %f) -> (%ld, %f), %ld +%ld)\n",
                 x1, y1, x2, y2, offset, count);

  for (i = 0; i < count; i++) {
    *data++ = y1 + (RemixPCM)(i + offset - x1) * (y2 - y1) / (x2 - x1);
  }

  return count;
}
