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
 * RemixStream: An indexed, sparse, polyphonic PCM data container.
 *
 * Conrad Parker <Conrad.Parker@CSIRO.AU>, August 2001
 *
 * Description
 * -----------
 *
 * A stream consists of multiple channels of PCM data.
 *
 * Invariants
 * ----------
 *
 * A stream is an independent entity.
 */

#define __REMIX__
#include "remix.h"

/* RemixStream */

/* Optimisation dependencies: none */
static RemixStream * remix_stream_optimise (RemixEnv * env,
					    RemixStream * stream);

RemixBase *
remix_stream_init (RemixEnv * env, RemixBase * base)
{
  RemixStream * stream = (RemixStream *)base;
  CDSet * s, * channels = remix_get_channels (env);

  stream->channels = cd_set_new (env);

  for (s = channels; s; s = s->next) {
    remix_stream_add_channel (env, stream, s->key);
  }

  remix_stream_optimise (env, stream);
  return (RemixBase *)stream;
}

static RemixStream *
remix_stream_add_channel_unchecked (RemixEnv * env, RemixStream * stream,
				    int name, RemixChannel * channel)
{
  stream->channels = cd_set_insert (env, stream->channels, name,
				    CD_POINTER(channel));
  return stream;
}

RemixChannel *
remix_stream_add_channel (RemixEnv * env, RemixStream * stream, int name)
{
  RemixChannel * channel = remix_stream_find_channel (env, stream, name);

  if (channel == RemixNone) {
    channel = remix_channel_new (env);
    remix_stream_add_channel_unchecked (env, stream, name, channel);
  }

  return channel;
}

RemixStream *
remix_stream_new (RemixEnv * env)
{
  RemixStream * stream =
    (RemixStream *)remix_base_new_subclass (env, sizeof (struct _RemixStream));

  remix_stream_init (env, (RemixBase *)stream);

  return stream;
}

RemixStream *
remix_stream_new_contiguous (RemixEnv * env, RemixCount length)
{
  RemixStream * stream = remix_stream_new (env);
  remix_stream_add_chunks (env, stream, 0, length);
  return stream;
}

RemixStream *
remix_stream_new_from_buffers (RemixEnv * env, RemixCount length,
			       RemixPCM ** buffers)
{
  CDSet * s;
  RemixStream * stream = remix_stream_new (env);
  RemixChannel * channel;
  RemixChunk * chunk;
  int i = 0;

  for (s = stream->channels; s; s = s->next) {
    channel = (RemixChannel *)s->data.s_pointer;
    chunk = remix_chunk_new_from_buffer (env, 0, length, buffers[i++]);
    remix_channel_add_chunk (env, channel, chunk);
  }

  return stream;
}

static RemixBase *
remix_stream_clone (RemixEnv * env, RemixBase * base)
{
  RemixStream * stream = (RemixStream *)base;
  RemixStream * new_stream;
  CDSet * s;
  RemixChannel * channel, * new_channel;

  if (stream == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return RemixNone;
  }

  new_stream = (RemixStream *)remix_stream_new (env);

  /* clone channels */
  for (s = stream->channels; s; s = s->next) {
    channel = (RemixChannel *)s->data.s_pointer;
    new_channel = remix_channel_clone (env, channel);
    remix_stream_add_channel_unchecked (env, new_stream, s->key, new_channel);
  }
  return (RemixBase *)new_stream;
}

static int
remix_stream_destroy (RemixEnv * env, RemixBase * base)
{
  RemixStream * stream = (RemixStream *)base;

  if (stream == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  cd_set_destroy_with (env, stream->channels, (CDDestroyFunc)remix_destroy);

  remix_free (stream);
  return 0;
}

RemixCount
remix_stream_nr_channels (RemixEnv * env, RemixStream * stream)
{
  if (stream == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  return (RemixCount)cd_set_size (env, stream->channels);
}

RemixChannel *
remix_stream_find_channel (RemixEnv * env, RemixStream * stream, int name)
{
  CDScalar k;

  if (stream == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return RemixNone;
  }

  k = cd_set_find (env, stream->channels, name);
  return (RemixChannel *)k.s_pointer;
}


RemixStream *
remix_stream_remove_channel (RemixEnv * env, RemixStream * stream, int name)
{
  if (stream == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return RemixNone;
  }
   
  stream->channels = cd_set_remove (env, stream->channels, name);

  return stream;
}

RemixStream *
remix_stream_add_chunks (RemixEnv * env, RemixStream * stream, RemixCount offset,
		      RemixCount length)
{
  CDSet * s;
  RemixChannel * channel;

  if (stream == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return RemixNone;
  }

  for (s = stream->channels; s; s = s->next) {
    channel = (RemixChannel *)s->data.s_pointer;
    remix_channel_add_new_chunk (env, channel, offset, length);
  }

  return stream;
}

RemixCount
remix_stream_write0 (RemixEnv * env, RemixStream * stream, RemixCount count)
{
  CDSet * s;
  RemixChannel * channel;
  RemixCount offset;

  if (stream == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  offset = remix_tell (env, (RemixBase *)stream);

  for (s = stream->channels; s; s = s->next) {
    channel = (RemixChannel *)s->data.s_pointer;
    remix_channel_write0 (env, channel, count);
  }
  
  remix_seek (env, (RemixBase *)stream, offset + count, SEEK_SET);

  remix_dprintf ("[remix_stream_write0] (%p) written %ld\n", stream, count);

  return count;
}

/*
 * remix_stream_process (env, stream, count, input, output)
 *
 * RemixProcessFunc for RemixStream.
 * Ignores input. Copies data from 'stream' to 'output'.
 */
static RemixCount
remix_stream_process (RemixEnv * env, RemixBase * base, RemixCount count,
		   RemixStream * input, RemixStream * output)
{
  RemixStream * stream = (RemixStream *)base;
  return remix_stream_write (env, output, count, stream);
}

static RemixCount
remix_stream_length (RemixEnv * env, RemixBase * base)
{
  RemixStream * stream = (RemixStream *)base;
  RemixCount length, maxlength = 0;
  CDSet * s;
  RemixChannel * channel;

  if (stream == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  for (s = stream->channels; s; s = s->next) {
    channel = (RemixChannel *)s->data.s_pointer;
    length = _remix_channel_length (env, channel);
    maxlength = MAX (maxlength, length);
  }

  return maxlength;
}

static RemixCount
remix_stream_seek (RemixEnv * env, RemixBase * base, RemixCount offset)
{
  RemixStream * stream = (RemixStream *)base;
  CDSet * s;
  RemixChannel * channel;

  for (s = stream->channels; s; s = s->next) {
    channel = (RemixChannel *)s->data.s_pointer;
    _remix_channel_seek (env, channel, offset);
  }

  return offset;
}

static struct _RemixMethods _remix_stream_methods = {
  remix_stream_clone,
  remix_stream_destroy,
  NULL, /* ready */
  NULL, /* prepare */
  remix_stream_process,
  remix_stream_length,
  remix_stream_seek,
};

static RemixStream *
remix_stream_optimise (RemixEnv * env, RemixStream * stream)
{
  _remix_set_methods (env, stream, &_remix_stream_methods);
  return stream;
}


/*
 * remix_stream_chunkfuncify (stream, count, func, data)
 *
 */
RemixCount
remix_stream_chunkfuncify (RemixEnv * env, RemixStream * stream,
			   RemixCount count,
			   RemixChunkFunc func, void * data)
{
  RemixCount n, minn = count, offset = remix_tell (env, (RemixBase *)stream);
  CDSet * s, * channels = remix_get_channels (env);
  RemixChannel * channel;

  for (s = channels; s; s = s->next) {
    remix_dprintf ("[remix_stream_chunkfuncify] thinking of channel %d\n",
		   s->key);
  }

  if (stream == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  remix_dprintf ("[remix_stream_chunkfuncify] (%p, +%ld) @ %ld\n",
		 stream, count, offset);

  for (s = stream->channels; s; s = s->next) {
    if (cd_set_contains (env, channels, s->key)) {
      channel = (RemixChannel *)s->data.s_pointer;
      n = remix_channel_chunkfuncify (env, channel, minn, func, s->key, data);
      minn = MIN (n, minn);
    } else {
      remix_dprintf ("[remix_stream_chunkfuncify] channel %d not funced\n", s->key);
    }
  }

  remix_seek (env, (RemixBase *)stream, offset + minn, SEEK_SET);

  return minn;
}

RemixCount
remix_stream_chunkchunkfuncify (RemixEnv * env, RemixStream * src,
				RemixStream * dest, RemixCount count,
				RemixChunkChunkFunc func, void * data)
{
  RemixCount n, minn = count;
  RemixCount src_offset = remix_tell (env, (RemixBase *)src);
  RemixCount dest_offset = remix_tell (env, (RemixBase *)dest);
  CDSet * s, * channels = remix_get_channels (env);
  RemixChannel * sch, * dch;

  if (dest == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  remix_dprintf ("[remix_stream_ccf...] (%p -> %p, +%ld), src @ %ld, dest @ %ld\n",
	      src, dest, count, src_offset, dest_offset);

  for (s = dest->channels; s; s = s->next) {
    if (cd_set_contains (env, channels, s->key)) {
      dch = (RemixChannel *)s->data.s_pointer;
      sch = remix_stream_find_channel (env, src, s->key);
      if (sch != RemixNone) {
	n = remix_channel_chunkchunkfuncify (env, sch, dch, count, func, s->key,
					  data);
	if (n == -1) {
	  return -1;
	}

	minn = MIN (n, minn);
      }
    }
  }

  remix_seek (env, (RemixBase *)src, src_offset + minn, SEEK_SET);
  remix_seek (env, (RemixBase *)dest, dest_offset + minn, SEEK_SET);

  return minn;
}

RemixCount
remix_stream_chunkchunkchunkfuncify (RemixEnv * env,
				  RemixStream * src1, RemixStream * src2,
				  RemixStream * dest, RemixCount count,
				  RemixChunkChunkChunkFunc func, void * data)
{
  RemixCount n, minn = count;
  RemixCount src1_offset = remix_tell (env, (RemixBase *)src1);
  RemixCount src2_offset = remix_tell (env, (RemixBase *)src2);
  RemixCount dest_offset = remix_tell (env, (RemixBase *)dest);
  CDSet * s, * channels = remix_get_channels (env);
  RemixChannel * s1ch, * s2ch, * dch;

  if (dest == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  for (s = dest->channels; s; s = s->next) {
    if (cd_set_contains (env, channels, s->key)) {
      dch = (RemixChannel *)s->data.s_pointer;
      s1ch = remix_stream_find_channel (env, src1, s->key);
      s2ch = remix_stream_find_channel (env, src2, s->key);
      if (s1ch != RemixNone && s2ch != RemixNone) {
	n = remix_channel_chunkchunkchunkfuncify (env, s1ch, s2ch, dch,
					       count, func, s->key, data);
	minn = MIN (n, minn);
      }
    }
  }

  remix_seek (env, (RemixBase *)src1, src1_offset + minn, SEEK_SET);
  remix_seek (env, (RemixBase *)src2, src2_offset + minn, SEEK_SET);
  remix_seek (env, (RemixBase *)dest, dest_offset + minn, SEEK_SET);

  return minn;
}

/*
 * remix_stream_gain (env, src, dest, count, gain)
 */
RemixCount
remix_stream_gain (RemixEnv * env, RemixStream * stream, RemixCount count, RemixPCM gain)
{
  return remix_stream_chunkfuncify (env, stream, count, _remix_chunk_gain, &gain);
}

/*
 * remix_stream_copy (env, src, dest, count)
 *
 * Copy 'count' samples from 'src' to 'dest'
 */
RemixCount
remix_stream_copy (RemixEnv * env, RemixStream * src, RemixStream * dest, RemixCount count)
{
  remix_dprintf ("[remix_stream_copy] (%p -> %p, +%ld)\n", src, dest, count);

  return remix_stream_chunkchunkfuncify (env, src, dest, count,
				      _remix_chunk_copy, NULL);
}

/*
 * remix_stream_write (env, stream, count, data)
 *
 * Write 'count' samples of 'data' to the stream 'stream'.
 */
/* XXX: this is a wrapper to obsolete remix_stream_write above */
RemixCount
remix_stream_write (RemixEnv * env, RemixStream * stream, RemixCount count,
		 RemixStream * data)
{
  remix_dprintf ("[remix_stream_write] (%p -> %p, +%ld)\n", data, stream, count);

  if (data == RemixNone)
    return remix_stream_write0 (env, stream, count);

  return remix_stream_copy (env, data, stream, count);
}

/*
 * remix_stream_mix (src, dest, count)
 *
 * Mix 'count' samples from 'src' into 'dest'.
 */
RemixCount
remix_stream_mix (RemixEnv * env, RemixStream * src, RemixStream * dest, RemixCount count)
{
  remix_dprintf ("[remix_stream_mix] (%p -> %p, +%ld)\n", src, dest, count);

  return remix_stream_chunkchunkfuncify (env, src, dest, count,
				      _remix_chunk_add_inplace, NULL);
}

/*
 * remix_stream_mult (src, dest, count)
 *
 * Multiply 'count' samples of 'src' into 'dest'.
 */
RemixCount
remix_stream_mult (RemixEnv * env, RemixStream * src, RemixStream * dest, RemixCount count)
{
  remix_dprintf ("[remix_stream_mult] (%p -> %p, +%ld)\n", src, dest, count);

  return remix_stream_chunkchunkfuncify (env, src, dest, count,
				      _remix_chunk_mult_inplace, NULL);
}

/*
 * remix_stream_fade (src, dest, count)
 *
 * Fade 'count' samples of 'dest' by values in 'src'.
 */
RemixCount
remix_stream_fade (RemixEnv * env, RemixStream * src, RemixStream * dest,
		  RemixCount count)
{
  remix_dprintf ("[remix_stream_fade] (%p -> %p, +%ld)\n", src, dest, count);

  return remix_stream_chunkchunkfuncify (env, src, dest, count,
					_remix_chunk_fade_inplace, NULL);
}

/*
 * remix_stream_blend (env, src, blend, dest, count)
 *
 * Blend 'count' samples of 'src' into 'dest' by amounts in 'blend'.
 */
RemixCount
remix_stream_blend (RemixEnv * env, RemixStream * src, RemixStream * blend,
		   RemixStream * dest, RemixCount count)
{
  remix_dprintf ("[remix_stream_blend] (%p -> (%p) -> %p, +%ld)\n", src, blend,
	      dest, count);

  return remix_stream_chunkchunkchunkfuncify (env, src, blend, dest, count,
					     _remix_chunk_blend_inplace, NULL);
}


/*
 * remix_streams_mix (streams, dest, count)
 *
 * Mix 'count' samples from all streams in list 'streams' together into
 * 'dest'.
 */
RemixCount
remix_streams_mix (RemixEnv * env, CDList * streams, RemixStream * dest,
		  RemixCount count)
{
  CDList * sl;
  CDSet * s;
  RemixChannel * sch, * dch;
  RemixStream * stream;
  RemixCount dest_start = remix_tell (env, (RemixBase *)dest);
  RemixCount stream_start;

  if (dest == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  for (s = dest->channels; s; s = s->next) {
    dch = (RemixChannel *)s->data.s_pointer;
    for (sl = streams; sl; sl = sl->next) {
      stream = (RemixStream *)sl->data.s_pointer;
      stream_start = remix_tell (env, (RemixBase *)stream);
      sch = remix_stream_find_channel (env, stream, s->key);
      if (sch != RemixNone) {
        _remix_channel_seek (env, dch, dest_start);
        remix_channel_mix (env, sch, dch, count);
        remix_seek (env, (RemixBase *)stream, stream_start, SEEK_SET);
      }
    }
  }

  for (sl = streams; sl; sl = sl->next) {
    stream = (RemixStream *)sl->data.s_pointer;
    stream_start = remix_tell (env, (RemixBase *)stream);
    remix_seek (env, (RemixBase *)stream, stream_start + count, SEEK_SET);
  }

  remix_seek (env, (RemixBase *)dest, dest_start+count, SEEK_SET);

  return count;
}

/*
 * remix_stream_interleave_2 (env, stream, name1, name2, dest, count)
 *
 * Interleave 'count' frames of the channels named 'name1' and 'name2' in
 * 'stream', placing the resulting PCM data in the memory region pointed
 * to by 'dest'.
 */
RemixCount
remix_stream_interleave_2 (RemixEnv * env, RemixStream * stream,
			  int name1, int name2,
			  RemixPCM * dest, RemixCount count)
{
  RemixChannel * channel1, * channel2;
  RemixCount n = 0;

  channel1 = remix_stream_find_channel (env, stream, name1);
  channel2 = remix_stream_find_channel (env, stream, name2);

  if (channel1 != RemixNone && channel2 != RemixNone) {
    n = remix_channel_chunkchunkfuncify (env, channel1, channel2,
					count, _remix_chunk_interleave_2,
					0, dest);
  }

  if (n > 0) remix_seek (env, (RemixBase *)stream, n, SEEK_CUR);

  return n;
}

/*
 * remix_stream_deinterleave_stereo (env, stream, name1, name2, src, count)
 *
 * Deinterleave 'count' frames from the memory region pointed to by 'src',
 * placing the resulting channels into those named 'name1' and 'name2' in
 * 'stream'.
 */
RemixCount
remix_stream_deinterleave_2 (RemixEnv * env, RemixStream * stream,
			    int name1, int name2,
			    RemixPCM * src, RemixCount count)
{
  RemixChannel * channel1, * channel2;
  RemixCount n = 0;

  channel1 = remix_stream_find_channel (env, stream, name1);
  channel2 = remix_stream_find_channel (env, stream, name2);

  if (channel1 != RemixNone && channel2 != RemixNone) {
    n = remix_channel_chunkchunkfuncify (env, channel1, channel2,
					count, _remix_chunk_deinterleave_2,
					0, src);
  }

  if (n > 0) remix_seek (env, (RemixBase *)stream, n, SEEK_CUR);

  return n;
}
