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
 * RemixChannel: An indexed, sparse, monophonic PCM data container.
 *
 * Conrad Parker <Conrad.Parker@CSIRO.AU>, August 2001
 *
 * Description
 * -----------
 *
 * A channel contains a sequence of chunks which contain raw PCM data.
 *
 * Channels are named, such that when streams are mixed together their
 * correspondingly named channels are mixed together.
 *
 * A channel is indexed by sample count. When reading a channel, all
 * index points for which no chunk is defined are defined as zero. Writing
 * to a channel stops early if no chunk is available at a required point.
 *
 * If two or more chunks overlap, the chunk with the latest start index
 * is always used for both reading and writing data values for the region
 * of overlap.
 *
 * Invariants
 * ----------
 *
 * A channel must be contained within a stream.
 */

#define __REMIX__
#include "remix.h"

/* RemixChannel */

RemixChannel *
remix_channel_new (RemixEnv * env)
{
  RemixChannel * c;

  c = (RemixChannel *) remix_malloc (sizeof (struct _RemixChannel));

  c->chunks = cd_list_new (env);
  c->_current_offset = 0;
  c->_current_chunk = RemixNone;

  return c;
}

RemixChannel *
remix_channel_clone (RemixEnv * env, RemixChannel * channel)
{
  RemixChannel * new_channel = remix_channel_new (env);
  new_channel->chunks = cd_list_clone (env, channel->chunks,
				       (CDCloneFunc)remix_chunk_clone);
  return new_channel;
}

int
remix_channel_destroy (RemixEnv * env, RemixBase * base)
{
  RemixChannel * channel = (RemixChannel *)base;
  CDList * l;

  for (l = channel->chunks; l; l = l->next) {
    remix_chunk_free (env, (RemixChunk *)l->data.s_pointer);
  }

  cd_list_free (env, channel->chunks);
  remix_free (channel);
  return 0;
}

RemixChunk *
remix_channel_add_chunk (RemixEnv * env, RemixChannel * channel,
			 RemixChunk * chunk)
{
  channel->chunks = cd_list_insert (env, channel->chunks, CD_TYPE_POINTER,
                                    CD_POINTER(chunk),
				    (CDCmpFunc)remix_chunk_later);
  return chunk;
}

RemixChunk *
remix_channel_add_new_chunk (RemixEnv * env, RemixChannel * channel,
			     RemixCount offset, RemixCount length)
{
  RemixChunk * chunk = remix_chunk_new (env, offset, length);
  return remix_channel_add_chunk (env, channel, chunk);
}

void
remix_channel_remove_chunk (RemixEnv * env, RemixChannel * channel,
			    RemixChunk * chunk)
{
  channel->chunks = cd_list_remove (env, channel->chunks, CD_TYPE_POINTER,
				    CD_POINTER(chunk));
}

RemixChunk *
remix_channel_find_chunk_before (RemixEnv * env, RemixChannel * channel,
				 RemixCount index)
{
  CDList * l;
  RemixChunk * u, * up = RemixNone;

  for (l = channel->chunks; l; l = l->next) {
    u = (RemixChunk *)l->data.s_pointer;
    if (u->start_index > index) return up;
    up = u;
  }

  return up;
}

/*
 * remix_chunk_item_valid_length (l)
 *
 * Returns the length for which the chunk item 'l' is valid. The valid
 * length of a chunk item in a channel is defined as the minimum of
 * (the chunk's actual length) and (the difference between the offset
 * of the chunk and the offset of the following chunk). ie. if the
 * chunk is followed by another chunk that cuts it off early, the valid
 * length is the difference between the two chunk offsets. Crikey, email
 * me if you're confused.
 */
static RemixCount
remix_chunk_item_valid_length (CDList * l)
{
  CDList * ln = l->next;
  RemixChunk * u = (RemixChunk *)l->data.s_pointer, * un;

  if (ln == RemixNone) {
    return u->length;
  } else {
    un = (RemixChunk *)ln->data.s_pointer;
    return MIN (u->length, un->start_index - u->start_index);
  }
}

RemixChunk *
remix_channel_get_chunk_at (RemixEnv * env, RemixChannel * channel,
			    RemixCount offset)
{
  CDList * l;
  RemixChunk * u;
  RemixCount vl; /* valid length */

  for (l = channel->chunks; l; l = l->next) {
    u = (RemixChunk *)l->data.s_pointer;
    vl = remix_chunk_item_valid_length (l);
    if (u->start_index <= offset && u->start_index + vl > offset) {
      /* chunk validly spans offset: return it */
      return u;
    }
  }

  /* No chunks found spanning offset */
  return RemixNone;
}

CDList *
remix_channel_get_chunk_item_at (RemixChannel * channel, RemixCount offset)
{
  CDList * l;
  RemixChunk * u;
  RemixCount vl; /* valid length */

  for (l = channel->chunks; l; l = l->next) {
    u = (RemixChunk *)l->data.s_pointer;
    vl = remix_chunk_item_valid_length (l);
    if (u->start_index <= offset && u->start_index + vl > offset) {
      /* chunk validly spans offset: return its item */
      return l;
    }
  }

  /* No chunks found spanning offset */
  return RemixNone;
}

CDList *
remix_channel_get_chunk_item_after (RemixChannel * channel, RemixCount offset)
{
  CDList * l;
  RemixChunk * u;

  for (l = channel->chunks; l; l = l->next) {
    u = (RemixChunk *)l->data.s_pointer;
    if (u->start_index >= offset) return l;
  }

  return RemixNone;
}

RemixCount
remix_channel_write0 (RemixEnv * env, RemixChannel * channel, RemixCount length)
{
  CDList * l = channel->_current_chunk;
  RemixChunk * u;
  RemixCount remaining = length, n, vl;
  RemixCount offset = channel->_current_offset;

  while (remaining > 0) {
    if (l == RemixNone) break; /* No more chunks */

    u = (RemixChunk *)l->data.s_pointer;

    if (u->start_index > offset) { /* skip ahead to start of next chunk */
      n = MIN (remaining, u->start_index - offset);
      offset += n;
      remaining -= n;
    }

    if (remaining > 0) {
      vl = remix_chunk_item_valid_length (l);
      n = _remix_chunk_clear_region (env, u, offset, MIN(remaining, vl),
				  0, NULL);
      offset += n;
      remaining -= n;
    }

    l = l->next;
  }

  channel->_current_chunk = l;
  channel->_current_offset += length;

  return length;
}

/*
 * remix_channel_chunkfuncify (env, channel, count, func, data)
 *
 * Apply the RemixChunkFunc func to 'count' samples from consecutive chunks
 * of 'channel'.
 * Stops early if the channel runs out of chunks.
 * Returns the number of samples func'ed.
 */
RemixCount
remix_channel_chunkfuncify (RemixEnv * env, RemixChannel * channel,
			    RemixCount count, RemixChunkFunc func,
			    int channelname, void * data)
{
  RemixChunk * u;
  RemixCount remaining = count, funced = 0, n, vl;
  RemixError error;

  remix_dprintf ("[remix_channel_chunkfuncify] (%p, +%ld) @ %ld\n",
		 channel, count, channel->_current_offset);

  while (remaining > 0) {
    channel->_current_chunk =
      remix_channel_get_chunk_item_at (channel, channel->_current_offset);
    if (channel->_current_chunk == RemixNone) {
      remix_dprintf ("[remix_channel_chunkfuncify] channel incomplete, funced %ld\n",
		     funced);
      return funced; /* Channel incomplete */
    }

    u = (RemixChunk *)channel->_current_chunk->data.s_pointer;
    vl = remix_chunk_item_valid_length (channel->_current_chunk);

    n = func (env, u, channel->_current_offset, MIN(remaining, vl),
	      channelname, data);

    if (n == -1) {
      error = remix_last_error (env);
      switch (error) {
      case REMIX_ERROR_SILENCE:
	n = _remix_chunk_clear_region (env, u, channel->_current_offset,
				       MIN(remaining, vl), 0, NULL);
	break;
      default:
	n = 0;
      }
    }

    funced += n;
    remaining -= n;
    channel->_current_offset += n;
  }

  return funced;
}

/*
 * remix_channel_chunkchunkfuncify (env, src, dest, count, func, data)
 *
 * Apply the RemixChunkChunkFunc func to corresponding chunks of 'src' and
 * 'dest' to 'count' samples.
 * Stops early if 'dest' cannot contain part of the region for which
 * 'src' is defined. Copies zeroes to 'dest' wherever 'src' is empty.
 * Returns the number of samples func'ed.
 */
RemixCount
remix_channel_chunkchunkfuncify (RemixEnv * env, RemixChannel * src, RemixChannel * dest,
			      RemixCount count, RemixChunkChunkFunc func,
			      int channelname, void * data)
{
  RemixChunk * su, * du;
  RemixCount remaining = count, funced = 0, n, vl;
  RemixError error;

  remix_dprintf ("[remix_channel_ccf...] (%p -> %p, +%ld), src @ %ld, dest @ %ld\n",
	  src, dest, count, src->_current_offset, dest->_current_offset);

  while (remaining > 0) {
    n = 0; /* watch for early changes to n */

    dest->_current_chunk =
	remix_channel_get_chunk_item_at (dest, dest->_current_offset);
    if (dest->_current_chunk == RemixNone) {
      remix_dprintf ("[remix_channel_ccf...] channel incomplete after %ld\n", funced);
      return funced; /* Destination channel incomplete */
    }

    src->_current_chunk =
	remix_channel_get_chunk_item_at (src, src->_current_offset);
    if (src->_current_chunk == RemixNone) { /* No source data at offset */
      src->_current_chunk =
	remix_channel_get_chunk_item_after (src, src->_current_offset);

      if (src->_current_chunk == RemixNone) {
        /* No following source data at all */
	remix_dprintf ("[remix_channel_ccf...] no source data after %ld\n",
		    src->_current_offset);
	n = remix_channel_write0 (env, dest, remaining);
	funced += n;
	remaining -= n;
	return funced;
      } else {
	remix_dprintf ("[remix_channel_ccf...] no source data at %ld\n",
		    src->_current_offset);
      }
    }

    /* *** Now, sl is the current or following source chunk item *** */

    su = (RemixChunk *)src->_current_chunk->data.s_pointer;

    if (su->start_index > src->_current_offset) { /* No source data at offset */
      remix_dprintf ("[remix_channel_ccf...] no source data at %ld (warn 2)\n",
		  src->_current_offset);
      n = remix_channel_write0 (env, dest, su->start_index - src->_current_offset);
      funced += n;
      remaining -= n;
      src->_current_offset += n;
    }

    if (remaining > 0) {
      if (n > 0) {
	dest->_current_chunk =
	  remix_channel_get_chunk_item_at (dest, dest->_current_offset);
	if (dest->_current_chunk == RemixNone) {
	  remix_dprintf ("[remix_channel_ccf...] dest incomplete after %ld\n",
		      dest->_current_chunk);
	  return funced; /* Destination channel incomplete */
	}
      }
 
      du = (RemixChunk *)dest->_current_chunk->data.s_pointer;

      vl = remix_chunk_item_valid_length (dest->_current_chunk);
      n = func (env, su, src->_current_offset, du, dest->_current_offset,
                MIN(remaining, vl), channelname, data);

      if (n == -1) {
	error = remix_last_error (env);
	switch (error) {
	case REMIX_ERROR_SILENCE:
	  n = _remix_chunk_clear_region (env, du, dest->_current_offset,
				      MIN(remaining, vl), 0, NULL);
	  break;
	default:
	  n = 0;
	}
      }

      funced += n;
      remaining -= n;
      src->_current_offset += n;
      dest->_current_offset += n;
    }
  }

  remix_dprintf ("[remix_channel_ccf...] funced %ld\n", funced);

  return funced;
}

/*
 * remix_channel_chunkchunkchunkfuncify (env, src1, src2, dest, count, func,
 *                                    data)
 *
 * Apply the RemixChunkChunkChunkFunc func to corresponding chunks of 'src1',
 * 'src2' and 'dest' to 'count' samples.
 * Stops early if 'dest' cannot contain part of the region for which
 * both 'src1' and 'src2' is defined. Copies zeroes to 'dest' wherever
 * either 'src1' or 'src2' are empty.
 * Returns the number of samples func'ed.
 */
RemixCount
remix_channel_chunkchunkchunkfuncify (RemixEnv * env,
				   RemixChannel * src1, RemixChannel * src2,
				   RemixChannel * dest, RemixCount count,
				   RemixChunkChunkChunkFunc func,
				   int channelname, void * data)
{
  RemixChunk * s1u, * s2u, * du;
  RemixCount remaining = count, funced = 0, n, vl, undef_length;
  RemixError error;

  while (remaining > 0) {
    n = 0; /* watch for early changes to n */

    dest->_current_chunk =
      remix_channel_get_chunk_item_at (dest, dest->_current_offset);
    if (dest->_current_chunk == RemixNone)
      return funced; /* Destination channel incomplete */

    src1->_current_chunk =
      remix_channel_get_chunk_item_at (src1, src1->_current_offset);
    if (src1->_current_chunk == RemixNone) {
      src1->_current_chunk =
	remix_channel_get_chunk_item_after (src1, src1->_current_offset);
    }

    src2->_current_chunk =
      remix_channel_get_chunk_item_at (src2, src2->_current_offset);
    if (src2->_current_chunk == RemixNone) {
      src2->_current_chunk =
	remix_channel_get_chunk_item_after (src2, src2->_current_offset);
    }

    if (src1->_current_chunk == RemixNone || src2->_current_chunk == RemixNone) {
      n = remix_channel_write0 (env, dest, remaining);
      funced += n;
      remaining -= n;
      return funced;
    }

    s1u = (RemixChunk *)src1->_current_chunk->data.s_pointer;
    s2u = (RemixChunk *)src2->_current_chunk->data.s_pointer;

    if (s1u->start_index > src1->_current_offset ||
	s2u->start_index > src2->_current_offset) {
      undef_length = MAX (s1u->start_index - src1->_current_offset,
			  s2u->start_index - src2->_current_offset);
      n = remix_channel_write0 (env, dest, undef_length);
      funced += n;
      remaining -= n;
      src1->_current_offset += n;
      src2->_current_offset += n;
    }
    
    if (remaining > 0) {
      if (n > 0) {
	dest->_current_chunk =
          remix_channel_get_chunk_item_at (dest, dest->_current_offset);
	if (dest->_current_chunk == RemixNone)
          return funced; /* Destination channel incomplete */
      }
      
      du = (RemixChunk *)dest->_current_chunk->data.s_pointer;
      
      vl = remix_chunk_item_valid_length (dest->_current_chunk);
      n = func (env, s1u, src1->_current_offset, s2u, src2->_current_offset,
		du, dest->_current_offset, MIN(remaining, vl), channelname,
		data);

      if (n == -1) {
	error = remix_last_error (env);
	switch (error) {
	case REMIX_ERROR_SILENCE:
	  n = _remix_chunk_clear_region (env, du, dest->_current_offset,
				      MIN(remaining, vl), 0, NULL);
	  break;
	default:
	  n = 0;
	}
      }

      funced += n;
      remaining -= n;
      src1->_current_offset += n;
      src2->_current_offset += n;
      dest->_current_offset += n;
    }
  }

  return funced;
}

/*
 * remix_channel_copy (src, dest, count)
 *
 * Copies 'count' samples from 'src' to 'dest'.
 * Stops early if 'dest' cannot contain part of 'src'. Copies zeroes to
 * 'dest' wherever 'src' is correspondingly empty.
 * Returns the count of samples actually copied.
 */
RemixCount
remix_channel_copy (RemixEnv * env, RemixChannel * src, RemixChannel * dest, RemixCount count)
{
  return
    remix_channel_chunkchunkfuncify (env, src, dest, count,
				     (RemixChunkChunkFunc)_remix_chunk_copy,
				     0, NULL);
}

RemixCount
remix_channel_mix (RemixEnv * env, RemixChannel * src, RemixChannel * dest, RemixCount count)
{
  return remix_channel_chunkchunkfuncify (env, src, dest, count,
                                       (RemixChunkChunkFunc)_remix_chunk_add_inplace,
				       0, NULL);
}

RemixCount
remix_channel_interleave_2 (RemixEnv * env, RemixChannel * src1, RemixChannel * src2,
			 RemixPCM * dest, RemixCount count)
{
  return
    remix_channel_chunkchunkfuncify (env, src1, src2, count,
				  (RemixChunkChunkFunc)_remix_chunk_interleave_2,
				  0, dest);
}

RemixCount
remix_channel_deinterleave_2 (RemixEnv * env, RemixChannel * dest1, RemixChannel * dest2,
			   RemixPCM * src, RemixCount count)
{
  return
    remix_channel_chunkchunkfuncify (env, dest1, dest2, count,
				  (RemixChunkChunkFunc)_remix_chunk_deinterleave_2,
				  0, src);
}

RemixCount
_remix_channel_write (RemixEnv * env, RemixChannel * channel, RemixCount count,
		   RemixChannel * data)
{
  RemixCount n = remix_channel_copy (env, data, channel, count);
  data->_current_offset += n;
  data->_current_chunk =
    remix_channel_get_chunk_item_after (data, data->_current_offset);
  channel->_current_offset += n;
  channel->_current_chunk =
    remix_channel_get_chunk_item_after (channel, channel->_current_offset);
  return n;
}

RemixCount
_remix_channel_length (RemixEnv * env, RemixChannel * channel)
{
  RemixChunk * last = (RemixChunk *)
    (cd_list_last (env, channel->chunks, CD_TYPE_POINTER)).s_pointer;
  if (last == RemixNone) return 0;
  return (last->start_index + last->length);
}

RemixCount
_remix_channel_seek (RemixEnv * env, RemixChannel * channel, RemixCount offset)
{
  if (offset == channel->_current_offset) return offset;
  channel->_current_offset = offset;
  /* Cache the current chunk */
  channel->_current_chunk =
    remix_channel_get_chunk_item_after (channel, offset);
  return offset;
}
