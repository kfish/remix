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
 * RemixTime: A generic time abstraction for sequencing information.
 *
 * Conrad Parker <conrad@metadecks.org>, August 2001
 */

#define __REMIX__
#include "remix.h"

RemixTime
remix_time_zero (RemixTimeType type)
{
  return _remix_time_zero (type);
}

RemixTime
remix_time_invalid (RemixTimeType type)
{
  return _remix_time_invalid (type);
}

int
remix_time_is_invalid (RemixTimeType type, RemixTime time)
{
  return _remix_time_is_invalid (type, time);
}

RemixTime
remix_time_add (RemixTimeType type, RemixTime t1, RemixTime t2)
{
  return _remix_time_add (type, t1, t2);
}

RemixTime
remix_time_sub (RemixTimeType type, RemixTime t1, RemixTime t2)
{
  return _remix_time_sub (type, t1, t2);
}

RemixTime
remix_time_min (RemixTimeType type, RemixTime t1, RemixTime t2)
{
  return _remix_time_min (type, t1, t2);
}

RemixTime
remix_time_max (RemixTimeType type, RemixTime t1, RemixTime t2)
{
  return _remix_time_max (type, t1, t2);
}

int
remix_time_eq (RemixTimeType type, RemixTime t1, RemixTime t2)
{
  return _remix_time_eq (type, t1, t2);
}

int
remix_time_gt (RemixTimeType type, RemixTime t1, RemixTime t2)
{
  return _remix_time_gt (type, t1, t2);
}

int
remix_time_lt (RemixTimeType type, RemixTime t1, RemixTime t2)
{
  return _remix_time_lt (type, t1, t2);
}

int
remix_time_ge (RemixTimeType type, RemixTime t1, RemixTime t2)
{
  return _remix_time_ge (type, t1, t2);
}

int
remix_time_le (RemixTimeType type, RemixTime t1, RemixTime t2)
{
  return _remix_time_le (type, t1, t2);
}


static float
remix_samples_to_seconds (RemixCount samples, RemixSamplerate samplerate)
{
  return ((float)samples / samplerate);
}

static int
remix_samples_to_beat24s (RemixCount samples, RemixSamplerate samplerate,
		       RemixTempo tempo)
{
  return ((int)((float)samples * tempo * 24.0 / (samplerate * 60.0)));
}

static RemixCount
remix_seconds_to_samples (float seconds, RemixSamplerate samplerate)
{
  return (RemixCount)(seconds * samplerate);
}

static int
remix_seconds_to_beat24s (float seconds, RemixTempo tempo)
{
  return (int)(seconds * tempo * 24.0 / 60.0);
}

static RemixCount
remix_beat24s_to_samples (int beat24s, RemixSamplerate samplerate,
                          RemixTempo tempo)
{
  return (RemixCount)(beat24s * samplerate * 60.0 / (tempo * 24.0));
}

static float
remix_beat24s_to_seconds (int beat24s, RemixTempo tempo)
{
  return ((float)beat24s * 60.0 / (tempo * 24.0));
}


RemixTime
remix_time_convert (RemixEnv * env, RemixTime time, RemixTimeType old_type,
                    RemixTimeType new_type)
{
  RemixSamplerate samplerate;
  RemixTempo tempo;

  if (old_type == new_type) return time;

  samplerate = remix_get_samplerate (env);
  tempo = remix_get_tempo (env);

  switch (old_type) {
  case REMIX_TIME_SAMPLES:
    switch (new_type) {
    case REMIX_TIME_SECONDS:
      return (RemixTime) remix_samples_to_seconds (time.samples, samplerate);
      break;
    case REMIX_TIME_BEAT24S:
      return (RemixTime)
        remix_samples_to_beat24s (time.samples, samplerate, tempo);
      break;
    default: break;
    }
    break;
  case REMIX_TIME_SECONDS:
    switch (new_type) {
    case REMIX_TIME_SAMPLES:
      return (RemixTime) remix_seconds_to_samples (time.seconds, samplerate);
      break;
    case REMIX_TIME_BEAT24S:
      return (RemixTime) remix_seconds_to_beat24s (time.seconds, tempo);
      break;
    default: break;
    }
    break;
  case REMIX_TIME_BEAT24S:
    switch (new_type) {
    case REMIX_TIME_SAMPLES:
      return (RemixTime)
        remix_beat24s_to_samples (time.beat24s, samplerate, tempo);
      break;
    case REMIX_TIME_SECONDS:
      return (RemixTime) remix_beat24s_to_seconds (time.beat24s, tempo);
      break;
    default: break;
    }
    break;
  default:
    /* Cannot convert invalid time type to anything */
    break;
  }

  return remix_time_invalid (new_type);
}
