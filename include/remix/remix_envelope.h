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

#ifndef __REMIX_ENVELOPE_H__
#define __REMIX_ENVELOPE_H__

/** \file
 *
 * The information describing how a parameter changes over time appears
 * as a generic data source. In order to create this mix automation information
 * Remix provides linear and spline envelopes.
 * However, parameters could alternatively be controlled by other means such
 * as from a recording of physical slider values, from a sine wave
 * generator, or from a deck constructed solely to generate interesting
 * parameter values.
 */

#include <remix/remix_types.h>

#if defined(__cplusplus)
extern "C" {
#endif

/* Envelopes */
RemixEnvelope * remix_envelope_new (RemixEnv * env, RemixEnvelopeType type);
RemixEnvelopeType remix_envelope_set_type (RemixEnv * env,
					   RemixEnvelope * envelope,
					   RemixEnvelopeType type);
RemixEnvelopeType remix_envelope_get_type (RemixEnv * env, RemixEnvelope * envelope);
RemixTimeType remix_envelope_set_timetype (RemixEnv * env,
					   RemixEnvelope * envelope,
					   RemixTimeType timetype);
RemixTimeType remix_envelope_get_timetype (RemixEnv * env,
					   RemixEnvelope * envelope);
RemixPCM remix_envelope_get_value (RemixEnv * env, RemixEnvelope * envelope,
				   RemixTime time);
RemixTime remix_envelope_get_duration (RemixEnv * env,
				       RemixEnvelope * envelope);
RemixPCM remix_envelope_get_integral (RemixEnv * env, RemixEnvelope * envelope,
				      RemixTime t1, RemixTime t2);
RemixPoint * remix_envelope_add_point (RemixEnv * env,
				       RemixEnvelope * envelope,
				       RemixTime time, RemixPCM value);
RemixEnvelope * remix_envelope_remove_point (RemixEnv * env,
					     RemixEnvelope * envelope,
					     RemixPoint * point);
RemixEnvelope * remix_envelope_scale (RemixEnv * env, RemixEnvelope * envelope,
				      RemixPCM gain);
RemixEnvelope * remix_envelope_shift (RemixEnv * env, RemixEnvelope * envelope,
				      RemixTime delta);

#if defined(__cplusplus)
}
#endif

#endif /* __REMIX_H__ */
