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
 * RemixSndfile: a libsndfile handler
 *
 * Conrad Parker <Conrad.Parker@CSIRO.AU>, August 2001
 */

#include <stdio.h>
#include <sndfile.h>
#include <string.h>

#define __REMIX__
#include "remix.h"

#define PATH_KEY 1
#define BLOCK_FRAMES 4096

typedef struct _RemixSndfileInstance RemixSndfileInstance;

struct _RemixSndfileInstance {
  char * path;
  int writing;
  SNDFILE * file;
  SF_INFO info;
  float * pcm;
  sf_count_t pcm_n;
};


/* Optimisation dependencies: none */
static RemixBase * remix_sndfile_optimise (RemixEnv * env, RemixBase * sndfile);


static RemixBase *
remix_sndfile_create (RemixEnv * env, RemixBase * sndfile,
		     const char * path, int writing)
{
  RemixSndfileInstance * si =
    remix_malloc (sizeof (struct _RemixSndfileInstance));

  si->path = strdup (path);
  si->writing = writing;

  if (writing) {
    si->info.samplerate = remix_get_samplerate (env);
    si->info.channels = 1; /* XXX: how many channels, or specify? */
    si->info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16; /* XXX: assumes WAV */

    si->file = sf_open  (path, SFM_WRITE, &si->info);
    si->pcm = NULL;
    si->pcm_n = 0;
  } else {
    si->file = sf_open (path, SFM_READ, &si->info);
    si->pcm = (float *) malloc (BLOCK_FRAMES * si->info.channels *
				sizeof(float));
    si->pcm_n = 0;
  }

  if (si->file == NULL) {
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    remix_destroy (env, (RemixBase *)sndfile);
    return RemixNone;
  }

  sf_command (si->file, SFC_SET_NORM_FLOAT, NULL, SF_TRUE);

  if (writing)
    sf_command (si->file, SFC_SET_ADD_DITHER_ON_WRITE, NULL, SF_TRUE);

  sndfile->instance_data = si;

  return sndfile;
}

static RemixBase *
remix_sndfile_reader_init (RemixEnv * env, RemixBase * base, CDSet * parameters)
{
  char * path;

  path = (cd_set_find (env, parameters, PATH_KEY)).s_string;

  if (remix_sndfile_create (env, base, path, 0) == RemixNone)
  	return RemixNone;

  remix_sndfile_optimise (env, base);
  return base;
}

static RemixBase *
remix_sndfile_writer_init (RemixEnv * env, RemixBase * base, CDSet * parameters)
{
  char * path;

  path = (cd_set_find (env, parameters, PATH_KEY)).s_string;

  remix_sndfile_create (env, base, path, 1);
  remix_sndfile_optimise (env, base);
  return base;
}

static RemixBase *
remix_sndfile_clone (RemixEnv * env, RemixBase * base)
{
  RemixBase * new_sndfile = remix_base_new (env);
  RemixSndfileInstance * si = (RemixSndfileInstance *)base->instance_data;
  remix_sndfile_create (env, new_sndfile, si->path, si->writing);
  remix_sndfile_optimise (env, new_sndfile);
  return new_sndfile;
}

static int
remix_sndfile_destroy (RemixEnv * env, RemixBase * base)
{
  RemixSndfileInstance * si = (RemixSndfileInstance *)base->instance_data;
  if (si->file != NULL) sf_close (si->file);
  remix_free (si);
  remix_free (base);
  return 0;
}

/* An RemixChunkFunc for creating sndfile */

static RemixCount
remix_sndfile_read_update (RemixEnv * env, RemixBase * sndfile,
			   RemixCount count)
{
  RemixSndfileInstance * si = (RemixSndfileInstance *)sndfile->instance_data;

  si->pcm_n = sf_readf_float (si->file, si->pcm, count);

  return si->pcm_n;
}

static RemixCount
remix_sndfile_read_into_chunk (RemixEnv * env, RemixChunk * chunk,
			       RemixCount offset, RemixCount count,
			       int channelname, void * data)
{
  RemixBase * sndfile = (RemixBase *)data;
  RemixPCM * d, * p;
  RemixCount remaining = count, written = 0, n, i;
  RemixSndfileInstance * si = (RemixSndfileInstance *)sndfile->instance_data;

  remix_dprintf ("[remix_sndfile_read_into_chunk] (%p, +%ld) @ %ld\n",
		 sndfile, count, remix_tell (env, sndfile));

  d = &chunk->data[offset];

  n = MIN (remaining, BLOCK_FRAMES);
  if (channelname == 0)
    remix_sndfile_read_update (env, sndfile, n);

  n = MIN (si->pcm_n, remaining);
  
  p = si->pcm;
  p += channelname;
  
  for (i = 0; i < n; i++) {
    *d++ = *p;
    p += si->info.channels;
  }
    
  if (n == 0) { /* EOF */
    n = _remix_pcm_set (d, 0.0, remaining);
  }
  
  remaining -= n;
  written += n;
  
#if 0 /* mono only */
  d = &chunk->data[offset];

  while (remaining > 0) {
    n = MIN (remaining, BLOCK_FRAMES);
    n = sf_readf_float (si->file, d, n);

    if (n == 0) { /* EOF */
      n = _remix_pcm_set (d, 0.0, remaining);
    }

    remaining -= n;
    written += n;

    d += n;
  }
#endif

  return written;
}

static RemixCount
remix_sndfile_write_from_chunk (RemixEnv * env, RemixChunk * chunk,
			       RemixCount offset, RemixCount count,
			       int channelname, void * data)
{
  RemixBase * sndfile = (RemixBase *)data;
  RemixPCM * d;
  RemixCount remaining = count, read = 0, n;
  RemixSndfileInstance * si = (RemixSndfileInstance *) sndfile->instance_data;

  remix_dprintf ("[remix_sndfile_write_from_chunk] (%p, +%ld) @ %ld\n",
		 sndfile, count, remix_tell (env, sndfile));

  d = &chunk->data[offset];

  while (remaining > 0) {
    n = MIN (remaining, BLOCK_FRAMES);
    n = sf_write_float (si->file, d, n);

    if (n == 0) { /* EOF */
      n = remaining;
    }

    remaining -= n;
    read += n;

    d += n;
  }

  return read;
}

static RemixCount
remix_sndfile_reader_process (RemixEnv * env, RemixBase * base,
			      RemixCount count,
			      RemixStream * input, RemixStream * output)
{
  return remix_stream_chunkfuncify (env, output, count,
				    remix_sndfile_read_into_chunk, base);
}

static RemixCount
remix_sndfile_writer_process (RemixEnv * env, RemixBase * base,
			      RemixCount count,
			      RemixStream * input, RemixStream * output)
{
  return remix_stream_chunkfuncify (env, output, count,
				   remix_sndfile_write_from_chunk, base);
}

static RemixCount
remix_sndfile_length (RemixEnv * env, RemixBase * base)
{
  RemixSndfileInstance * si = (RemixSndfileInstance *)base->instance_data;
  return si->info.frames;
}

static RemixCount
remix_sndfile_seek (RemixEnv * env, RemixBase * base, RemixCount offset)
{
  RemixSndfileInstance * si = (RemixSndfileInstance *)base->instance_data;
  return sf_seek (si->file, offset, SEEK_SET);
}

static struct _RemixMethods _remix_sndfile_reader_methods = {
  remix_sndfile_clone,
  remix_sndfile_destroy,
  NULL, /* ready */
  NULL, /* prepare */
  remix_sndfile_reader_process,
  remix_sndfile_length,
  remix_sndfile_seek,
  NULL, /* flush */
};

static struct _RemixMethods _remix_sndfile_writer_methods = {
  remix_sndfile_clone,
  remix_sndfile_destroy,
  NULL, /* ready */
  NULL, /* prepare */
  remix_sndfile_writer_process,
  remix_sndfile_length,
  remix_sndfile_seek,
  NULL, /* flush */
};  

static RemixBase *
remix_sndfile_optimise (RemixEnv * env, RemixBase * sndfile)
{
  RemixSndfileInstance * si = (RemixSndfileInstance *)sndfile->instance_data;

  if (si->writing)
    remix_base_set_methods (env, sndfile, &_remix_sndfile_writer_methods);
  else
    remix_base_set_methods (env, sndfile, &_remix_sndfile_reader_methods);

  return sndfile;
}


static struct _RemixParameterScheme path_scheme = {
  "path",
  "Path to sound file",
  REMIX_TYPE_STRING,
  REMIX_CONSTRAINT_TYPE_NONE,
  REMIX_CONSTRAINT_EMPTY,
  REMIX_HINT_FILENAME,
};

#if 0
static RemixNamedParameter types[] = {
  REMIX_NAMED_PARAMETER ("WAV", CD_INT(SF_FORMAT_WAV)),
  REMIX_NAMED_PARAMETER ("AIFF", CD_INT(SF_FORMAT_AIFF)),
  REMIX_NAMED_PARAMETER ("Sun/NeXT AU format (big endian)", CD_INT(SF_FORMAT_AU)),
  REMIX_NAMED_PARAMETER ("DEC AU format (little endian)", CD_INT(SF_FORMAT_AULE)),
  REMIX_NAMED_PARAMETER ("RAW PCM data", CD_INT(SF_FORMAT_RAW)),
  REMIX_NAMED_PARAMETER ("Ensoniq PARIS", CD_INT(SF_FORMAT_PAF)),
  REMIX_NAMED_PARAMETER ("Amiga IFF / SVX8 / SV16", CD_INT(SF_FORMAT_SVX)),
  REMIX_NAMED_PARAMETER ("Sphere NIST", CD_INT(SF_FORMAT_NIST)),
  REMIX_NAMED_PARAMETER ("Windows Media Audio", CD_INT(SF_FORMAT_WMA)),
  REMIX_NAMED_PARAMETER ("Sekd Samplitude", CD_INT(SF_FORMAT_SMPLTD)),
  REMIX_NAMED_PARAMETER ("VOC", CD_INT(SF_FORMAT_VOC)),
  REMIX_NAMED_PARAMETER ("Sound Designer 2", CD_INT(SF_FORMAT_SD2)),
  REMIX_NAMED_PARAMETER ("Rex2", CD_INT(SF_FORMAT_REX2)),
  REMIX_NAMED_PARAMETER ("PCM", CD_INT(SF_FORMAT_PCM)),
  REMIX_NAMED_PARAMETER ("32 bit floats", CD_INT(SF_FORMAT_FLOAT)),
  REMIX_NAMED_PARAMETER ("U-Law encoded", CD_INT(SF_FORMAT_ULAW)),
  REMIX_NAMED_PARAMETER ("A-Law encoded", CD_INT(SF_FORMAT_ALAW)),
  REMIX_NAMED_PARAMETER ("IMA ADPCM", CD_INT(SF_FORMAT_IMA_ADPCM)),
  REMIX_NAMED_PARAMETER ("Microsoft ADPCM", CD_INT(SF_FORMAT_MS_ADPCM)),
  REMIX_NAMED_PARAMETER ("Big endian PCM", CD_INT(SF_FORMAT_PCM_BE)),
  REMIX_NAMED_PARAMETER ("Little endian PCM", CD_INT(SF_FORMAT_PCM_LE)),
  REMIX_NAMED_PARAMETER ("Signed 8 bit PCM", CD_INT(SF_FORMAT_PCM_S8)),
  REMIX_NAMED_PARAMETER ("Unsigned 8 bit PCM", CD_INT(SF_FORMAT_PCM_U8)),
  REMIX_NAMED_PARAMETER ("SVX Fibonacci Delta", CD_INT(SF_FORMAT_SVX_FIB)),
  REMIX_NAMED_PARAMETER ("SVX Exponential Delta", CD_INT(SF_FORMAT_SVX_EXP)),
  REMIX_NAMED_PARAMETER ("GSM 6.10 Encoding", CD_INT(SF_FORMAT_GSM610)),
  REMIX_NAMED_PARAMETER ("32kbs G721 ADPCM", CD_INT(SF_FORMAT_G721_32)),
  REMIX_NAMED_PARAMETER ("24kbs G723 ADPCM", CD_INT(SF_FORMAT_G723_24)),
};
#endif

static struct _RemixParameterScheme format_scheme = {
  "format",
  "Format of sound file",
  REMIX_TYPE_INT,
  REMIX_CONSTRAINT_TYPE_LIST,
  REMIX_CONSTRAINT_EMPTY,
  REMIX_HINT_DEFAULT,
};

static struct _RemixMetaText sndfile_reader_metatext = {
  "builtin::sndfile_reader",
  "File::Sndfile Reader",
  "Reads PCM audio files using libsndfile",
  "Copyright (C) 2001, 2002 CSIRO Australia",
  "http://www.metadecks.org/software/env/plugins/sndfile.html",
  REMIX_ONE_AUTHOR ("Conrad Parker", "Conrad.Parker@CSIRO.AU"),
};

static struct _RemixMetaText sndfile_writer_metatext = {
  "builtin::sndfile_writer",
  "File::Sndfile Writer",
  "Writes PCM audio files using libsndfile",
  "Copyright (C) 2001, 2002 CSIRO Australia",
  "http://www.metadecks.org/software/env/plugins/sndfile.html",
  REMIX_ONE_AUTHOR ("Conrad Parker", "Conrad.Parker@CSIRO.AU"),
};

static struct _RemixPlugin sndfile_reader_plugin = {
  &sndfile_reader_metatext,
  REMIX_FLAGS_NONE,
  CD_EMPTY_SET, /* init scheme */
  remix_sndfile_reader_init,
  CD_EMPTY_SET, /* process scheme */
  NULL, /* suggests */
  NULL, /* plugin data */
};

static struct _RemixPlugin sndfile_writer_plugin = {
  &sndfile_writer_metatext,
  REMIX_FLAGS_NONE,
  CD_EMPTY_SET, /* init scheme */
  remix_sndfile_writer_init,
  CD_EMPTY_SET, /* process scheme */
  NULL, /* suggests */
  NULL, /* plugin data */
};

/* module init function */
CDList *
__sndfile_init (RemixEnv * env)
{
  CDList * plugins = cd_list_new (env);
  int i, count;
  SF_FORMAT_INFO info;
  RemixNamedParameter * param;

  sndfile_reader_plugin.init_scheme =
    cd_set_insert (env, sndfile_reader_plugin.init_scheme, PATH_KEY,
		   CD_POINTER(&path_scheme));

  plugins = cd_list_prepend (env, plugins,
			     CD_POINTER(&sndfile_reader_plugin));

  format_scheme.constraint.list = cd_list_new (env);

  sf_command (NULL, SFC_GET_FORMAT_MAJOR_COUNT, &count, sizeof (int)) ;

  for (i = 0; i < count ; i++) {
    info.format = i ;
    sf_command (NULL, SFC_GET_FORMAT_MAJOR, &info, sizeof (info)) ;

    param = remix_malloc (sizeof(RemixNamedParameter));
    param->name = strdup (info.name);
    param->parameter = CD_INT(info.format);

    format_scheme.constraint.list =
      cd_list_append (env, format_scheme.constraint.list, CD_POINTER(param));
  }

  sndfile_writer_plugin.init_scheme =
    cd_set_insert (env, sndfile_writer_plugin.init_scheme, PATH_KEY,
		   CD_POINTER(&path_scheme));

  plugins = cd_list_prepend (env, plugins,
			     CD_POINTER(&sndfile_writer_plugin));

  return plugins;
}

