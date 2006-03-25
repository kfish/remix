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

/** \file
 * Base types and public interfaces to libremix
 *
 */

/*
 * Conrad Parker <conrad@metadecks.org>, August 2001
 */

#ifndef __REMIX_TYPES_H__
#define __REMIX_TYPES_H__

#include <limits.h>

#include "ctxdata.h"

#define RemixNone NULL

typedef int RemixError;

typedef float RemixPCM;

typedef long RemixCount;
#define REMIX_COUNT_MAX (LONG_MAX - 1L)
#define REMIX_COUNT_MIN LONG_MIN
#define REMIX_COUNT_INFINITE REMIX_COUNT_MAX

typedef void RemixOpaque;

#if defined (__REMIX__)
typedef struct _RemixBase RemixBase;
#else
typedef RemixOpaque RemixBase;
#endif

typedef CDScalar RemixParameter;

typedef int RemixFlags;
typedef double RemixSamplerate;
typedef double RemixTempo;
typedef union _RemixTime RemixTime;


/* Errors */
#define REMIX_ERROR_OK              0
#define REMIX_ERROR_INVALID         1
#define REMIX_ERROR_NOENTITY        2
#define REMIX_ERROR_EXISTS          3
#define REMIX_ERROR_SILENCE         4
#define REMIX_ERROR_NOOP            5
#define REMIX_ERROR_SYSTEM          6

typedef enum {
  REMIX_CHANNEL_LEFT,
  REMIX_CHANNEL_RIGHT,
  REMIX_CHANNEL_CENTRE,
  REMIX_CHANNEL_REAR,
  REMIX_CHANNEL_REAR_LEFT,
  REMIX_CHANNEL_REAR_RIGHT,
  REMIX_CHANNEL_REAR_CENTRE,
  REMIX_CHANNEL_LFE /* Low Frequency Effects */
} RemixChannelName;

typedef enum {
  REMIX_TIME_INVALID,
  REMIX_TIME_SAMPLES,
  REMIX_TIME_SECONDS,
  REMIX_TIME_BEAT24S
} RemixTimeType;

/* Envelope types */
typedef enum {
  REMIX_ENVELOPE_LINEAR,
  REMIX_ENVELOPE_SPLINE
} RemixEnvelopeType;

union _RemixTime {
  long TIME;
  RemixCount samples;
  float seconds;
  int beat24s;
};

typedef enum {
  REMIX_TYPE_BOOL = 0,
  REMIX_TYPE_INT,
  REMIX_TYPE_FLOAT,
  REMIX_TYPE_STRING,
  REMIX_TYPE_BASE
} RemixParameterType;

typedef enum {
  REMIX_CONSTRAINT_TYPE_NONE = 0,
  REMIX_CONSTRAINT_TYPE_LIST,
  REMIX_CONSTRAINT_TYPE_RANGE
} RemixConstraintType;


#define REMIX_RANGE_LOWER_BOUND_VALID (1<<0)
#define REMIX_RANGE_UPPER_BOUND_VALID (1<<1)
#define REMIX_RANGE_STEP_VALID        (1<<2)
#define REMIX_RANGE_ALL_VALID (REMIX_RANGE_LOWER_BOUND_VALID | \
                              REMIX_RANGE_UPPER_BOUND_VALID | \
                              REMIX_RANGE_STEP_VALID)

#define REMIX_HINT_DEFAULT  (0)
#define REMIX_HINT_LOG      (1<<0)
#define REMIX_HINT_TIME     (1<<1)
#define REMIX_HINT_FILENAME (1<<2)

#define REMIX_CONSTRAINT_EMPTY ((RemixConstraint){NULL})

#define REMIX_SAMPLES(x) ((RemixTime){(RemixCount)(x)})
#define REMIX_SECONDS(x) ((RemixTime){(float)(x)})
#define REMIX_BEAT24S(x) ((RemixTime){(int)(x)})


#if (defined(__REMIX__) || defined(__REMIX_PLUGIN__))
#include <remix/remix_plugin.h>
#else
typedef RemixOpaque RemixEnv;
typedef RemixOpaque RemixPoint;
typedef RemixOpaque RemixEnvelope;
typedef RemixOpaque RemixChunk;
typedef RemixOpaque RemixChannel;
typedef RemixOpaque RemixStream;
typedef RemixOpaque RemixDeck;
typedef RemixOpaque RemixTrack;
typedef RemixOpaque RemixLayer;
typedef RemixOpaque RemixSound;
typedef RemixOpaque RemixMetaAuthor;
typedef RemixOpaque RemixMetaText;
typedef RemixOpaque RemixPlugin;
typedef RemixOpaque RemixSquareTone;
typedef RemixOpaque RemixMonitor;
#endif

#if defined(__cplusplus)
}
#endif

#endif /* __REMIX_TYPES_H__ */
