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
 * RemixChunk: A contiguous chunk of monophonic PCM data.
 *
 * Conrad Parker <Conrad.Parker@CSIRO.AU>, August 2001
 *
 * Description
 * -----------
 *
 * A chunk contains raw PCM data.
 *
 * Invariants
 * ----------
 *
 * A chunk must always be contained within a channel. The data within
 * a chunk is only valid where it is not overlapped by a later chunk
 * in the same channel; elsewhere, the chunk's data is not used.
 *
 */

#include <string.h>

#define __REMIX__
#include "remix.h"

typedef RemixCount
  (*RemixPFunc) (RemixPCM * src, RemixCount count, void * data);

typedef RemixCount
  (*RemixPPFunc) (RemixPCM * src, RemixPCM * dest, RemixCount count,
			     void * data);
typedef RemixCount
  (*RemixPPPFunc) (RemixPCM * src1, RemixPCM * src2, RemixPCM * dest,
                   RemixCount count, void * data);


RemixChunk *
remix_chunk_new (RemixEnv * env, RemixCount start_index, RemixCount length)
{
  RemixChunk * u;

  u = (RemixChunk *) remix_malloc (sizeof (struct _RemixChunk));
  u->start_index = start_index;
  u->length = length;

  u->data = (RemixPCM *) remix_malloc (length * sizeof (RemixPCM));

  return u;
}

RemixChunk *
remix_chunk_new_from_buffer (RemixEnv * env, RemixCount start_index,
                             RemixCount length, RemixPCM * buffer)
{
  RemixChunk * u;

  u = (RemixChunk *) remix_malloc (sizeof (struct _RemixChunk));
  u->start_index = start_index;
  u->length = length;

  u->data = buffer;

  return u;
}

RemixChunk *
remix_chunk_clone (RemixEnv * env, RemixChunk * chunk)
{
  RemixChunk * u = remix_chunk_new (env, chunk->start_index, chunk->length);
  memcpy (u->data, chunk->data, (chunk->length * sizeof (RemixPCM)));
  return u;
}

void
remix_chunk_free (RemixEnv * env, RemixChunk * chunk)
{
  remix_free (chunk->data);
  remix_free (chunk);
}

int
remix_chunk_later (RemixEnv * env, RemixChunk * u1, RemixChunk * u2)
{
  return (u1->start_index > u2->start_index);
}

RemixCount
remix_chunk_clear (RemixEnv * env, RemixChunk * chunk)
{
  RemixCount len = chunk->length;
  memset (chunk->data, (RemixPCM)0, len * sizeof (RemixPCM));
  return len;
}


/*
 * FUNCTION APPLIERS
 */

static RemixCount
_remix_pfunc_apply (RemixEnv * env, RemixPFunc func, RemixChunk * chunk,
                    RemixCount start, RemixCount count, void * data)
{
  RemixCount chunk_start = start - chunk->start_index;

  /* chunk_start: 'start' relative to chunk */
  if (chunk_start < 0) {
    count += chunk_start;
    chunk_start = 0;
  }

  if (chunk_start + count > chunk->length) {
    count = chunk->length - chunk_start;
  }

  func (&chunk->data[chunk_start], count, data);

  return (count);
}

/*
 * _remix_ppfunc_apply (env, func, src, src_offset, dest, dest_offset, count,
 *                      data)
 *
 * Apply RemixPCMPCMFunc 'func' to 'count' samples of chunks 'src' and 'dest'.
 */
static RemixCount
_remix_ppfunc_apply (RemixEnv * env, RemixPPFunc func,
                     RemixChunk * src, RemixCount src_offset,
                     RemixChunk * dest, RemixCount dest_offset,
                     RemixCount count, void * data)
{
  RemixCount dest_start = dest_offset - dest->start_index;
  RemixCount src_start;
  RemixPCM *s, *d;

  if (dest_start < 0) {
    count += dest_start;
    dest_offset -= dest_start;
    src_offset -= dest_start;
    dest_start = 0;
  }

  src_start = src_offset - src->start_index;

  /* Shorten count if either chunk is shorter than required */
  if (src_start + count > src->length)
    count = src->length - src_start;

  if (dest_start + count > dest->length)
    count = dest->length - dest_start;

  s = &src->data[src_start];
  d = &dest->data[dest_start];

  func (s, d, count, data);

  return count;
}

/*
 * _remix_pppfunc_apply (env, func, src, src_offset, dest, dest_offset, count,
 *                       data)
 *
 * Apply RemixPPPFunc 'func' to 'count' samples of chunks 'src1', 'src2'
 * and 'dest'.
 */
static RemixCount
_remix_pppfunc_apply (RemixEnv * env, RemixPPPFunc func,
                      RemixChunk * src1, RemixCount src1_offset,
                      RemixChunk * src2, RemixCount src2_offset,
                      RemixChunk * dest, RemixCount dest_offset,
                      RemixCount count, void * data)
{
  RemixCount dest_start = dest_offset - dest->start_index;
  RemixCount src1_start, src2_start;
  RemixPCM *s1, *s2, *d;

  if (dest_start < 0) {
    count += dest_start;
    dest_offset -= dest_start;
    src1_offset -= dest_start;
    src2_offset -= dest_start;
    dest_start = 0;
  }

  src1_start = src1_offset - src1->start_index;
  src2_start = src2_offset - src2->start_index;

  /* Shorten count if any chunk is shorter than required */
  if (src1_start + count > src1->length)
    count = src1->length - src1_start;

  if (src2_start + count > src2->length)
    count = src2->length - src2_start;

  if (dest_start + count > dest->length)
    count = dest->length - dest_start;

  s1 = &src1->data[src1_start];
  s2 = &src2->data[src2_start];
  d = &dest->data[dest_start];

  func (s1, s2, d, count, data);

  return count;
}

/* PFunc appliers */

/*
 * _remix_chunk_clear_region (chunk, start, length, unused)
 *
 * Clear 'chunk' from stream index 'start' for 'length' samples.
 * Returns the count of samples actually cleared.
 */
RemixCount
_remix_chunk_clear_region (RemixEnv * env, RemixChunk * chunk,
                           RemixCount start, RemixCount length,
                           int channelname, void * unused)
{
  return _remix_pfunc_apply (env,_remix_pcm_clear_region,
                             chunk, start, length, NULL);
}

/*
 * _remix_chunk_gain (chunk, start, length, gain)
 *
 * Multiply by gain all samples in 'chunk' from stream index 'start' for
 * 'length' samples.
 * Returns the count of samples modified.
 */
RemixCount
_remix_chunk_gain (RemixEnv * env, RemixChunk * chunk,
                   RemixCount start, RemixCount count,
                   int channelname, /* (RemixPCM *) */ void * gain)
{
  return _remix_pfunc_apply (env, _remix_pcm_gain, chunk, start, count, gain);
}


/* PPFunc appliers */

/*
 * _remix_chunk_copy (env, src+offset, dest+offset, count, channelname)
 *
 * Copy data from 'src' to 'dest' from stream index 'offset' for
 * 'count'. Returns the count of samples actually copied.
 */
RemixCount
_remix_chunk_copy (RemixEnv * env, RemixChunk * src, RemixCount src_offset,
                   RemixChunk * dest, RemixCount dest_offset, RemixCount count,
                   int channelname, void * unused)
{
  return _remix_ppfunc_apply (env, _remix_pcm_copy, src, src_offset,
                              dest, dest_offset, count, NULL);
}

/*
 * _remix_chunk_add_inplace (env, src+offset, dest+offset, count, channelname)
 *
 * Add data from 'src' to data in 'dest' from stream index 'start' for
 * 'count' samples. Returns the count of samples actually added.
 */
RemixCount
_remix_chunk_add_inplace (RemixEnv * env,
                          RemixChunk * src, RemixCount src_offset,
                          RemixChunk * dest, RemixCount dest_offset,
                          RemixCount count, int channelname, void * unused)
{
  return _remix_ppfunc_apply (env, _remix_pcm_add, src, src_offset,
			   dest, dest_offset, count, NULL);
			   
}

/*
 * _remix_chunk_mult_inplace (src, dest, start, count, unused)
 *
 * Multiply data of 'dest' by that of 'src' from stream index 'start' for
 * 'count' samples. Returns the count of samples actually multiplied.
 */
RemixCount
_remix_chunk_mult_inplace (RemixEnv * env,
                           RemixChunk * src, RemixCount src_offset,
                           RemixChunk * dest, RemixCount dest_offset,
                           RemixCount count, int channelname, void * unused)
{
  return _remix_ppfunc_apply (env, _remix_pcm_mult, src, src_offset,
			   dest, dest_offset, count, NULL);
}

/*
 * _remix_chunk_fade_inplace (src, src_offset, dest, dest_offset, count, unused)
 *
 * Fade data of 'dest' by that of 'src' from stream index 'start' for
 * 'count' samples. Returns the count of samples actually faded.
 */
RemixCount
_remix_chunk_fade_inplace (RemixEnv * env,
                           RemixChunk * src, RemixCount src_offset,
                           RemixChunk * dest, RemixCount dest_offset,
                           RemixCount count, int channelname, void * unused)
{
  return _remix_ppfunc_apply (env, _remix_pcm_fade, src, src_offset,
			   dest, dest_offset, count, NULL);
}

/*
 * _remix_chunk_interleave_2 (env, src1+offset, src2+offset, count, dest)
 *
 * Interleave data of 'src1' from stream index 'src1_offset' with that
 * of 'src2' from stream index 'src2_offset' for 'count' samples, storing
 * the resulting interleaved PCM data in 'dest'.
 *
 * Returns the count of sample frames interleaved.
 */
RemixCount
_remix_chunk_interleave_2 (RemixEnv * env,
                           RemixChunk * src1, RemixCount src1_offset,
                           RemixChunk * src2, RemixCount src2_offset,
                           RemixCount count, int unused, void * dest)
{
  return _remix_ppfunc_apply (env, _remix_pcm_interleave_2, src1, src1_offset,
                              src2, src2_offset, count, dest);
}

/*
 * _remix_chunk_deinterleave_2 (env, dest1+offset, dest2+offset, count, src)
 *
 * Deinterleave data of 'src' and store the result in streams 'dest1' (from
 * stream index 'dest1_offset') and 'dest2' (from stream index
 * 'dest2_offset').
 *
 * Returns the number of sample frames deinterleaved.
 */
RemixCount
_remix_chunk_deinterleave_2 (RemixEnv * env,
                             RemixChunk * dest1, RemixCount dest1_offset,
                             RemixChunk * dest2, RemixCount dest2_offset,
                             RemixCount count, int unused, void * src)
{
  return _remix_ppfunc_apply (env, _remix_pcm_deinterleave_2,
                              dest1, dest1_offset, dest2, dest2_offset,
                              count, src);
}

/* PPPFunc appliers */

/*
 * _remix_chunk_blend_inplace (env, src+offset, blend+offset,
 *                             dest+offset, count, unused)
 *
 * Blend data of 'src' into that of 'dest' by blend values given in
 * 'blend'.
 */
RemixCount
_remix_chunk_blend_inplace (RemixEnv * env,
                            RemixChunk * src, RemixCount src_offset,
                            RemixChunk * blend, RemixCount blend_offset,
                            RemixChunk * dest, RemixCount dest_offset,
                            RemixCount count, int channelname, void * unused)
{
  return _remix_pppfunc_apply (env, _remix_pcm_blend, src, src_offset,
                               blend, blend_offset, dest, dest_offset,
                               count, NULL);
}
