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

#ifndef __REMIX_TIME_H__
#define __REMIX_TIME_H__

#include <remix/remix_types.h>

#if defined(__cplusplus)
extern "C" {
#endif

/* Time */
RemixTime remix_time_convert (RemixEnv * env, RemixTime time,
			      RemixTimeType old_type, RemixTimeType new_type);
RemixTime remix_time_zero (RemixTimeType type);
RemixTime remix_time_invalid (RemixTimeType type);
int remix_time_is_invalid (RemixTimeType type, RemixTime time);
RemixTime remix_time_add (RemixTimeType type, RemixTime t1, RemixTime t2);
RemixTime remix_time_sub (RemixTimeType type, RemixTime t1, RemixTime t2);
RemixTime remix_time_min (RemixTimeType type, RemixTime t1, RemixTime t2);
RemixTime remix_time_max (RemixTimeType type, RemixTime t1, RemixTime t2);
int remix_time_eq (RemixTimeType type, RemixTime t1, RemixTime t2);
int remix_time_gt (RemixTimeType type, RemixTime t1, RemixTime t2);
int remix_time_lt (RemixTimeType type, RemixTime t1, RemixTime t2);
int remix_time_ge (RemixTimeType type, RemixTime t1, RemixTime t2);
int remix_time_le (RemixTimeType type, RemixTime t1, RemixTime t2);


#if defined(__cplusplus)
}
#endif

#endif /* __REMIX_TIME_H__ */
