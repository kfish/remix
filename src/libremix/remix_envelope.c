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
 * RemixEnvelope: Generic mathy envelopes for env.
 *
 * Conrad Parker <Conrad.Parker@CSIRO.AU>, August 2001
 */

#define __REMIX__
#include "remix.h"

/* Optimisation dependencies: optimise when adding and removing points
 * or changing envelope type */
static RemixEnvelope * remix_envelope_optimise (RemixEnv * env, RemixEnvelope * envelope);

static void
remix_envelope_debug (RemixEnv * env, RemixEnvelope * envelope)
{
#ifdef DEBUG
  CDList * l;
  RemixPoint * point;

  printf ("envelope %p\n", envelope);
  printf ("envelope->points: %p\n", envelope->points);

  /*  printf (" has %d points\n", cd_list_length (env, envelope->points));*/
  for (l = envelope->points; l; l = l->next) {
    point = (RemixPoint *)l->data.s_pointer;
    switch (envelope->timetype) {
    case REMIX_TIME_SAMPLES:
      printf ("%ld samples, %f\n", point->time.samples, point->value);
      break;
    case REMIX_TIME_SECONDS:
      printf ("%f seconds, %f\n", point->time.seconds, point->value);
      break;
    case REMIX_TIME_BEAT24S:
      printf ("%d beat24s, %f\n", point->time.beat24s, point->value);
      break;
    default:
      printf ("*** unknown envelope->timetype ***\n");
      break;
    }
  } 
#endif
}

static RemixPoint *
remix_point_new (RemixTime time, RemixPCM value)
{
  RemixPoint * point = remix_malloc (sizeof (struct _RemixPoint));
  point->time = time;
  point->value = value;
  return point;
}

static RemixPoint *
remix_point_clone (RemixEnv * env, RemixPoint * point)
{
  return remix_point_new (point->time, point->value);
}

static int
remix_point_later_X (RemixEnv * env, RemixPoint * p1, RemixPoint * p2)
{
  return _remix_time_gt(REMIX_TIME_SAMPLES, p1->time, p2->time);
}

static int
remix_point_later_S (RemixEnv * env, RemixPoint * p1, RemixPoint * p2)
{
  return _remix_time_gt(REMIX_TIME_SECONDS, p1->time, p2->time);
}

static int
remix_point_later_B (RemixEnv * env, RemixPoint * p1, RemixPoint * p2)
{
  return _remix_time_gt(REMIX_TIME_BEAT24S, p1->time, p2->time);
}

RemixBase *
remix_envelope_init (RemixEnv * env, RemixBase * base)
{
  RemixEnvelope * envelope = (RemixEnvelope *)base;
  envelope->type = REMIX_ENVELOPE_LINEAR;
  envelope->points = cd_list_new (env);
  remix_envelope_optimise (env, envelope);
  return (RemixBase *)envelope;
}

RemixEnvelope *
remix_envelope_new (RemixEnv * env, RemixEnvelopeType type)
{
  RemixEnvelope * envelope =
    (RemixEnvelope *)remix_base_new_subclass (env, sizeof (struct _RemixEnvelope));
  remix_envelope_init (env, (RemixBase *)envelope);
  envelope->type = type;
  remix_envelope_debug (env, envelope);
  return envelope;
}

RemixBase *
remix_envelope_clone (RemixEnv * env, RemixBase * base)
{
  RemixEnvelope * envelope = (RemixEnvelope *)base;
  RemixEnvelope * new_envelope = remix_envelope_new (env, envelope->type);
  new_envelope->points = cd_list_clone (env, envelope->points,
					(CDCloneFunc)remix_point_clone);
  remix_envelope_optimise (env, new_envelope);
  return (RemixBase *)new_envelope;
}

static int
remix_envelope_destroy (RemixEnv * env, RemixBase * base)
{
  RemixEnvelope * envelope = (RemixEnvelope *)base;
  cd_list_free_all (env, envelope->points);
  remix_free (envelope);
  return 0;
}

RemixEnvelopeType
remix_envelope_set_type (RemixEnv * env, RemixEnvelope * envelope, RemixEnvelopeType type)
{
  RemixEnvelopeType old = envelope->type;
  envelope->type = type;
  remix_envelope_optimise (env, envelope);
  return old;
}

RemixEnvelopeType
remix_envelope_get_type (RemixEnv * env, RemixEnvelope * envelope)
{
  return envelope->type;
}

RemixTimeType
remix_envelope_set_timetype (RemixEnv * env, RemixEnvelope * envelope,
			  RemixTimeType timetype)
{
  RemixTimeType old = envelope->timetype;
  envelope->timetype = timetype;
  return old;
}

RemixTimeType
remix_envelope_get_timetype (RemixEnv * env, RemixEnvelope * envelope)
{
  return envelope->timetype;
}

RemixTime
remix_envelope_get_duration (RemixEnv * env, RemixEnvelope * envelope)
{
  CDList * l = cd_list_last_item (env, envelope->points);
  RemixPoint * p;

  if (l == RemixNone) return _remix_time_zero (envelope->timetype);

  p = (RemixPoint *)l->data.s_pointer;
  if (p == RemixNone) return _remix_time_zero (envelope->timetype);

  return p->time;
}

RemixPoint *
remix_envelope_add_point (RemixEnv * env, RemixEnvelope * envelope, RemixTime time,
		       RemixPCM value)
{
  RemixPoint * point = remix_point_new (time, value);
  switch (envelope->timetype) {
  case REMIX_TIME_SAMPLES:
    envelope->points = cd_list_insert (env, envelope->points,
				       CD_TYPE_POINTER, CD_POINTER(point),
				       (CDCmpFunc)remix_point_later_X);
    break;
  case REMIX_TIME_SECONDS:
    envelope->points = cd_list_insert (env, envelope->points,
				       CD_TYPE_POINTER, CD_POINTER(point),
				       (CDCmpFunc)remix_point_later_S);
    break;
  case REMIX_TIME_BEAT24S:
    envelope->points = cd_list_insert (env, envelope->points,
				       CD_TYPE_POINTER, CD_POINTER(point),
				       (CDCmpFunc)remix_point_later_B);
    break;
  default: /* uncommon, we should hope */
    remix_free (point); point = RemixNone;
    break;
  }
  remix_envelope_debug (env, envelope);
  remix_envelope_optimise (env, envelope);
  return point;
}

RemixEnvelope *
remix_envelope_remove_point (RemixEnv * env, RemixEnvelope * envelope, RemixPoint * point)
{
  envelope->points = cd_list_remove (env, envelope->points,
				     CD_TYPE_POINTER, CD_POINTER(point));
  remix_free (point);
  remix_envelope_debug (env, envelope);
  remix_envelope_optimise (env, envelope);
  return envelope;
}

RemixEnvelope *
remix_envelope_scale (RemixEnv * env, RemixEnvelope * envelope, RemixPCM gain)
{
  CDList * l;
  RemixPoint * p;

  for (l = envelope->points; l; l = l->next) {
    p = (RemixPoint *)l->data.s_pointer;
    p->value *= gain;
  }

  return envelope;
}

RemixEnvelope *
remix_envelope_shift (RemixEnv * env, RemixEnvelope * envelope, RemixTime delta)
{
  CDList * l;
  RemixPoint * p;

  for (l = envelope->points; l; l = l->next) {
    p = (RemixPoint *)l->data.s_pointer;
    p->time = _remix_time_add (envelope->timetype, p->time, delta);
  }

  return envelope;
}

static CDList *
remix_envelope_point_item_before (RemixEnv * env, RemixEnvelope * envelope,
			       RemixCount offset)
{
  CDList * l, * lp = RemixNone;
  RemixPoint * point;
  RemixTime ptime;

  for (l = envelope->points; l; l = l->next) {
    point = (RemixPoint *)l->data.s_pointer;
    ptime = remix_time_convert (env, point->time, envelope->timetype,
			     REMIX_TIME_SAMPLES);
    if (ptime.samples > offset) break;
    lp = l;
  }
  return lp;
}

static RemixCount
remix_envelope_constant_write_chunk (RemixEnv * env, RemixChunk * chunk, RemixCount offset,
				  RemixCount count, int channelname, void * data)
{
  RemixEnvelope * envelope = (RemixEnvelope *)data;
  RemixPoint * point;
  RemixPCM value;
  RemixPCM * d;
  RemixCount n;

  point = (RemixPoint *)envelope->points->data.s_pointer;
  value = point->value;
  d = &chunk->data[offset];

  n = _remix_pcm_set (d, value, count);
  envelope->_current_offset += n;
  return n;
}

#if 0
static RemixCount
remix_envelope_spline_write_chunk (RemixEnv * env, RemixChunk * chunk, RemixCount offset,
				RemixCount count, int channelname, void * data)
{
  RemixEnvelope * envelope = (RemixEnvelope *)data;
  /* XXX: Implement ;) */
  return -1;
}
#endif

/* An RemixChunkFunc for creating envelope data */
static RemixCount
remix_envelope_linear_write_chunk (RemixEnv * env, RemixChunk * chunk, RemixCount offset,
				RemixCount count, int channelname, void * data)
{
  RemixEnvelope * envelope = (RemixEnvelope *)data;
  RemixCount remaining = count, written = 0;
  RemixCount pos = envelope->_current_offset;
  CDList * l, * nl;
  RemixPoint * point, * next_point;
  RemixCount px, npx, n;
  RemixTime t;
  RemixPCM py, npy, gradient;
  RemixPCM * d;

  remix_dprintf ("[remix_envelope_linear_write_chunk] (%ld, +%ld) @ %ld\n",
	  offset, count, pos);


  l = envelope->_current_point_item;

  if (l == RemixNone) {/* No points before start */
    l = envelope->points;
    if (l == RemixNone) {/* No points at all */
      n = _remix_chunk_clear_region (env, chunk, offset, count, 0, NULL);
      envelope->_current_offset += n;
      return n;
    }
  }

  nl = l->next;
  if (nl == RemixNone) {
    /* if the last point was before offset, and there were
     * more points, set l to the second last and nl to the last */
    nl = l; l = l->prev;
    if (l == RemixNone) {/* Constant envelope (one point) */
      return remix_envelope_constant_write_chunk (env, chunk, offset, count,
					       channelname, envelope);
    }
  }

  point = (RemixPoint *)l->data.s_pointer;
  t = remix_time_convert (env, point->time, envelope->timetype, REMIX_TIME_SAMPLES);
  px = t.samples;
  py = point->value;
  
  next_point = (RemixPoint *)nl->data.s_pointer;
  t = remix_time_convert (env, next_point->time, envelope->timetype,
		       REMIX_TIME_SAMPLES);
  npx = t.samples;
  npy = next_point->value;

  while (remaining > 0) {
    if (nl->next == RemixNone) {
      /* These are the last two points, so fill out with this gradient */
      n = remaining;
    } else {
      n = MIN (remaining, npx - pos);
    }
    gradient = (npy - py) / (RemixPCM)(npx - px);
    
    d = &chunk->data[offset];
    /*    _remix_pcm_write_linear (d, px - chunk->start_index, py, gradient, n);*/
    n = _remix_pcm_write_linear (d, px, py, npx, npy, pos, n);
    
    remaining -= n;
    written += n;
    pos += n;
    offset += n;
    
    if (remaining > 0) {
      l = nl; point = next_point; px = npx; py = npy;
      
      nl = nl->next;
      next_point = (RemixPoint *)nl->data.s_pointer;
      t = remix_time_convert (env, next_point->time, envelope->timetype,
			   REMIX_TIME_SAMPLES);
      npx = t.samples;
      npy = next_point->value;
    }
  }

  envelope->_current_point_item = l;
  envelope->_current_offset = pos;

  return written;
}

static RemixCount
remix_envelope_constant_process (RemixEnv * env, RemixBase * base, RemixCount count,
			      RemixStream * input, RemixStream * output)
{
  RemixEnvelope * envelope = (RemixEnvelope *)base;
  return remix_stream_chunkfuncify (env, output, count,
				 remix_envelope_constant_write_chunk, envelope);
}

static RemixCount
remix_envelope_spline_process (RemixEnv * env, RemixBase * base, RemixCount count,
			    RemixStream * input, RemixStream * output)
{
  return -1;
}

static RemixCount
remix_envelope_linear_process (RemixEnv * env, RemixBase * base, RemixCount count,
			    RemixStream * input, RemixStream * output)
{
  RemixEnvelope * envelope = (RemixEnvelope *)base;
  return remix_stream_chunkfuncify (env, output, count,
				 remix_envelope_linear_write_chunk, envelope);
}

static RemixCount
remix_envelope_process (RemixEnv * env, RemixBase * base, RemixCount count,
		     RemixStream * input, RemixStream * output)
{
  RemixEnvelope * envelope = (RemixEnvelope *)base;

  switch (envelope->type) {
  case REMIX_ENVELOPE_LINEAR:
    return remix_envelope_linear_process (env, base, count, input, output);
    break;
  case REMIX_ENVELOPE_SPLINE:
    return remix_envelope_spline_process (env, base, count, input, output);
    break;
  default:
    break;
  }
  remix_set_error (env, REMIX_ERROR_NOOP);
  return 0;
}

static RemixCount
remix_envelope_length (RemixEnv * env, RemixBase * base)
{
  RemixEnvelope * envelope = (RemixEnvelope *)base;
  RemixTime duration = remix_envelope_get_duration (env, envelope);
  RemixTime t = remix_time_convert (env, duration, envelope->timetype,
			      REMIX_TIME_SAMPLES);
  return t.samples;
}

static RemixCount
remix_envelope_seek (RemixEnv * env, RemixBase * base, RemixCount offset)
{
  RemixEnvelope * envelope = (RemixEnvelope *)base;
  envelope->_current_point_item =
    remix_envelope_point_item_before (env, envelope, offset);
  envelope->_current_offset = offset;
  return offset;
}

static struct _RemixMethods _remix_envelope_empty_methods = {
  remix_envelope_clone,
  remix_envelope_destroy,
  NULL, /* ready */
  NULL, /* prepare */
  remix_null_process,
  remix_null_length,
  remix_envelope_seek,
  NULL, /* flush */
};

static struct _RemixMethods _remix_envelope_constant_methods = {
  remix_envelope_clone,
  remix_envelope_destroy,
  NULL, /* ready */
  NULL, /* prepare */
  remix_envelope_constant_process,
  remix_envelope_length,
  remix_envelope_seek,
  NULL, /* flush */
};

static struct _RemixMethods _remix_envelope_linear_methods = {
  remix_envelope_clone,
  remix_envelope_destroy,
  NULL, /* ready */
  NULL, /* prepare */
  remix_envelope_linear_process,
  remix_envelope_length,
  remix_envelope_seek,
  NULL, /* flush */
};

static struct _RemixMethods _remix_envelope_spline_methods = {
  remix_envelope_clone,
  remix_envelope_destroy,
  NULL, /* ready */
  NULL, /* prepare */
  remix_envelope_spline_process,
  remix_envelope_length,
  remix_envelope_seek,
  NULL, /* flush */
};

static struct _RemixMethods _remix_envelope_methods = {
  remix_envelope_clone,
  remix_envelope_destroy,
  NULL, /* ready */
  NULL, /* prepare */
  remix_envelope_process,
  remix_envelope_length,
  remix_envelope_seek,
  NULL, /* flush */
};

static RemixEnvelope *
remix_envelope_optimise (RemixEnv * env, RemixEnvelope * envelope)
{
  if (cd_list_is_empty (env, envelope->points)) {
    _remix_set_methods (env, envelope, &_remix_envelope_empty_methods);
  } else if (cd_list_is_singleton (env, envelope->points)) {
    _remix_set_methods (env, envelope, &_remix_envelope_constant_methods);
  } else {
    switch (envelope->type) {
    case REMIX_ENVELOPE_LINEAR:
      _remix_set_methods (env, envelope, &_remix_envelope_linear_methods); break;
    case REMIX_ENVELOPE_SPLINE:
      _remix_set_methods (env, envelope, &_remix_envelope_spline_methods); break;
    default:
      _remix_set_methods (env, envelope, &_remix_envelope_methods); break;
    }
  }

  return envelope;
}
