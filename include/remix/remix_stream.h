/*
 * libremix -- An audio mixing and sequencing library.
 *
 * Copyright (C) 2001 Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO), Australia.
 * Copyright (C) 2003 Conrad Parker
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
 * Conrad Parker <conrad@metadecks.org>, August 2001
 */

#ifndef __REMIX_STREAM_H__
#define __REMIX_STREAM_H__

/** \file
 *
 * The abstraction of multichannel audio data in libremix is known as a
 * stream.
 * A stream may consist of multiple channels, each of which can consist
 * of an arbitrary number of sparsely placed chunks of raw audio data.
 * The channels are named with spatial names such as LEFT, RIGHT and CENTRE
 * as required for common home, studio and theatre environments.
 *
 * \image html streams.png
 * \image latex streams.eps "Inside a Remix stream" width=10cm
 *
 * Generic routines are provided for mixing, multiplying and blending
 * streams of data.
 *
 */

#include <remix/remix_types.h>

#if defined(__cplusplus)
extern "C" {
#endif

/* Streams */
RemixStream * remix_stream_new (RemixEnv * env);
RemixStream * remix_stream_new_contiguous (RemixEnv * env, RemixCount length);
RemixStream * remix_stream_new_from_buffers (RemixEnv * env, RemixCount length,
					     RemixPCM ** buffers);
RemixCount remix_stream_nr_channels (RemixEnv * env, RemixStream * stream);
RemixChannel * remix_stream_find_channel (RemixEnv * env,
					  RemixStream * stream, int name);
RemixChannel * remix_stream_add_channel (RemixEnv * env,
					 RemixStream * stream, int name);
RemixStream * remix_stream_remove_channel (RemixEnv * env,
					   RemixStream * stream, int name);
RemixStream * remix_stream_add_chunks (RemixEnv * env, RemixStream * stream,
				       RemixCount offset, RemixCount length);

RemixCount remix_stream_write0 (RemixEnv * env, RemixStream * stream,
				RemixCount count);
RemixCount remix_stream_write (RemixEnv * env, RemixStream * stream,
			       RemixCount count, RemixStream * data);
RemixCount remix_stream_copy (RemixEnv * env, RemixStream * src,
			      RemixStream * dest, RemixCount count);
RemixCount remix_stream_gain (RemixEnv * env, RemixStream * stream,
			      RemixCount count, RemixPCM gain);
RemixCount remix_stream_mix (RemixEnv * env, RemixStream * src,
			     RemixStream * dest, RemixCount count);
RemixCount remix_stream_mult (RemixEnv * env, RemixStream * src,
			      RemixStream * dest, RemixCount count);
RemixCount remix_streams_mix (RemixEnv * env, CDList * streams,
			      RemixStream * dest, RemixCount count);
RemixCount remix_stream_fade (RemixEnv * env, RemixStream * src,
			      RemixStream * dest, RemixCount count);
RemixCount remix_stream_blend (RemixEnv * env, RemixStream * src,
			       RemixStream * dest,
			       RemixStream * blend, RemixCount count);

RemixCount remix_stream_interleave_2 (RemixEnv * env, RemixStream * stream,
				      int name1, int name2,
				      RemixPCM * dest, RemixCount count);
RemixCount remix_stream_deinterleave_2 (RemixEnv * env, RemixStream * stream,
					int name1, int name2,
					RemixPCM * src, RemixCount count);

/* Chunks */
int remix_chunk_later (RemixEnv * env, RemixChunk * u1, RemixChunk * u2);
RemixCount remix_chunk_clear (RemixEnv * env, RemixChunk * chunk);

#if defined(__cplusplus)
}
#endif

#endif /* __REMIX_STREAM_H__ */
