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
 * Metadata for RemixBase objects
 */

/*
 * Conrad Parker <conrad@metadecks.org>, August 2001
 */

#ifndef __REMIX_META_H__
#define __REMIX_META_H__

char * remix_meta_text_get_identifier (RemixEnv * env, RemixMetaText * mt);
char * remix_meta_text_set_identifier (RemixEnv * env, RemixMetaText * mt,
				       char * identifier);
char * remix_meta_text_get_category (RemixEnv * env, RemixMetaText * mt);
char * remix_meta_text_set_category (RemixEnv * env, RemixMetaText * mt,
				     char * category);
char * remix_meta_text_get_description (RemixEnv * env, RemixMetaText * mt);
char * remix_meta_text_set_description (RemixEnv * env, RemixMetaText * mt,
					char * description);
char * remix_meta_text_get_copyright (RemixEnv * env, RemixMetaText * mt);
char * remix_meta_text_set_copyright (RemixEnv * env, RemixMetaText * mt,
				      char * copyright);
char * remix_meta_text_get_url (RemixEnv * env, RemixMetaText * mt);
char * remix_meta_text_set_url (RemixEnv * env, RemixMetaText * mt,
				char * url);
CDList * remix_meta_text_get_authors (RemixEnv * env, RemixMetaText * mt);
void remix_meta_text_add_author (RemixEnv * env, RemixMetaText * mt,
				 char * name, char * email);

#if defined(__cplusplus)
}
#endif

#endif /* __REMIX_META_H__ */
