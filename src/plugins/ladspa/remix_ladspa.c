/*
 * LADSPA wrapper plugin for libremix
 *
 * Copyright (C) 2000 Conrad Parker
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
 * This file assumes that both LADSPA and libremix are built with
 * an audio datatype of 'float'.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <math.h> /* for ceil() */

#include <dlfcn.h>

#define __REMIX_PLUGIN__
#include <remix/remix.h>

#include "ladspa.h"

/* Compile in support for inplace processing? */
#define _PROCESS_INPLACE

#ifdef _PROCESS_INPLACE
#define LADSPA_WRAPPER_IS_INPLACE_BROKEN(x) LADSPA_IS_INPLACE_BROKEN(x)
#else
#define LADSPA_WRAPPER_IS_INPLACE_BROKEN(x) (1L)
#endif

#define LADSPA_IS_CONTROL_INPUT(x) (LADSPA_IS_PORT_INPUT(x) && LADSPA_IS_PORT_CONTROL(x))
#define LADSPA_IS_AUDIO_INPUT(x) (LADSPA_IS_PORT_INPUT(x) && LADSPA_IS_PORT_AUDIO(x))
#define LADSPA_IS_CONTROL_OUTPUT(x) (LADSPA_IS_PORT_OUTPUT(x) && LADSPA_IS_PORT_CONTROL(x))
#define LADSPA_IS_AUDIO_OUTPUT(x) (LADSPA_IS_PORT_OUTPUT(x) && LADSPA_IS_PORT_AUDIO(x))

#define LADSPA_frames_to_bytes(f) (f * sizeof(LADSPA_Data))

static char * default_ladspa_path = "/usr/lib/ladspa:/usr/local/lib/ladspa:/opt/ladspa/lib";


/* Dummy control output, used to connect all LADSPA control outputs to */
static LADSPA_Data dummy_control_output;

static CDList * modules_list = CD_EMPTY_LIST;
static int ladspa_wrapper_initialised = FALSE;


typedef struct _RemixLADSPA RemixLADSPA;

struct _RemixLADSPA {
  unsigned long samplerate; /* samplerate initialised at */
  LADSPA_Descriptor * d;
  LADSPA_Handle * handle;
  LADSPA_Data * control_inputs;
};

static RemixBase * remix_ladspa_optimise (RemixEnv * env, RemixBase * base);

/*
 * is_usable (d)
 *
 * Determine if a LADSPA_Descriptor * d is usable by this remix ladspa
 * wrapper plugin. Currently this means that there is not more than 1
 * audio input or more than 1 audio output.
 */
static int
is_usable(const LADSPA_Descriptor * d)
{
  LADSPA_PortDescriptor pd;
  int i;
  int
    nr_ai=0, /* audio inputs */
    nr_ao=0; /* audio outputs */

  for (i=0; i < d->PortCount; i++) {
    pd = d->PortDescriptors[i];
    if (LADSPA_IS_AUDIO_INPUT(pd))
      nr_ai++;
    if (LADSPA_IS_AUDIO_OUTPUT(pd))
      nr_ao++;
  }

  /* Sanity checks */
  if (! d->run) return FALSE; /* plugin does nothing! */
  if (! d->instantiate) return FALSE; /* plugin cannot be instantiated */
  if (! d->connect_port) return FALSE; /* plugin cannot be wired up */

  if (nr_ao == 1 && nr_ai == 1) return TRUE;
  if (nr_ao == 0 && nr_ai == 1) return TRUE;
  if (nr_ao == 1 && nr_ai == 0) return TRUE;

  return FALSE;
}

static RemixParameterType
convert_type (const LADSPA_PortRangeHintDescriptor prhd)
{
  if (LADSPA_IS_HINT_TOGGLED(prhd))
    return REMIX_TYPE_BOOL;
  else if (LADSPA_IS_HINT_INTEGER(prhd))
    return REMIX_TYPE_INT;
  else
    return REMIX_TYPE_FLOAT;
}

static RemixFlags
get_valid_mask (const LADSPA_PortRangeHintDescriptor prhd)
{
  RemixFlags ret = 0;

  if (LADSPA_IS_HINT_BOUNDED_BELOW(prhd))
    ret &= REMIX_RANGE_LOWER_BOUND_VALID;
  if (LADSPA_IS_HINT_BOUNDED_ABOVE(prhd))
    ret &= REMIX_RANGE_UPPER_BOUND_VALID;

  return ret;
}

static RemixParameterRange *
convert_constraint (const LADSPA_PortRangeHint * prh)
{
  RemixParameterRange * pr;
  LADSPA_PortRangeHintDescriptor prhd = prh->HintDescriptor;

  if (LADSPA_IS_HINT_TOGGLED(prhd))
    return NULL;

  pr = malloc (sizeof (*pr));

  pr->valid_mask = get_valid_mask (prhd);

  if (LADSPA_IS_HINT_INTEGER(prhd)) {
    if (LADSPA_IS_HINT_BOUNDED_BELOW(prhd))
      pr->lower.s_int = (int)prh->LowerBound;
    if (LADSPA_IS_HINT_BOUNDED_ABOVE(prhd))
      pr->upper.s_int = (int)prh->UpperBound;
  } else {
    if (LADSPA_IS_HINT_BOUNDED_BELOW(prhd))
      pr->lower.s_float = (float)prh->LowerBound;
    if (LADSPA_IS_HINT_BOUNDED_ABOVE(prhd))
      pr->upper.s_float = (float)prh->UpperBound;
  }

  return pr;
}

static RemixBase *
remix_ladspa_replace_handle (RemixEnv * env, RemixBase * base)
{
  RemixPlugin * plugin = remix_base_get_plugin (env, base);
  RemixLADSPA * al = (RemixLADSPA *) remix_base_get_instance_data (env, base);
  LADSPA_Descriptor * d;

  if (al == NULL) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return RemixNone;
  }

  if (al->d != NULL)
    if (al->d->deactivate) al->d->deactivate (al->handle);

  al->samplerate = (unsigned long) remix_get_samplerate (env);

  al->d = d = (LADSPA_Descriptor *) plugin->plugin_data;

  if (d != NULL) {
    al->handle = d->instantiate (d, al->samplerate);
    if (d->activate) d->activate (al->handle);
  }

  return base;
}

static RemixBase *
remix_ladspa_init (RemixEnv * env, RemixBase * base, CDSet * parameters)
{
  RemixLADSPA * al = malloc (sizeof (*al));

  remix_base_set_instance_data (env, base, al);
  al->d = NULL; /* let this get set in replace_handle() */

  remix_ladspa_replace_handle (env, base);
  remix_ladspa_optimise (env, base);
  return base;
}

static RemixBase *
remix_ladspa_clone (RemixEnv * env, RemixBase * base)
{
  /* XXX: Most of this should really be handled in remix_base.c */
  RemixPlugin * plugin = remix_base_get_plugin (env, base);
  RemixBase * new_base = remix_base_new (env);
  remix_base_set_plugin (env, new_base, plugin);
  remix_ladspa_init (env, new_base, CD_EMPTY_SET);
  return new_base;
}

static int
remix_ladspa_destroy (RemixEnv * env, RemixBase * base)
{
  RemixLADSPA * al = (RemixLADSPA *) remix_base_get_instance_data (env, base);

  if (al->d) {
    if (al->d->deactivate) al->d->deactivate (al->handle);
  }

  free (al->control_inputs);
  free (al);
  free (base);

  return 0;
}

static int
remix_ladspa_ready (RemixEnv * env, RemixBase * base)
{
  unsigned long samplerate = (unsigned long)remix_get_samplerate (env);
  RemixLADSPA * al = (RemixLADSPA *) remix_base_get_instance_data (env, base);
  return (samplerate == al->samplerate);
}

static RemixBase *
remix_ladspa_prepare (RemixEnv * env, RemixBase * base)
{
  remix_ladspa_replace_handle (env, base);
  return base;

}

static RemixCount
remix_ladspa_1_0 (RemixEnv * env, RemixChunk * chunk, RemixCount offset,
	       RemixCount count, int channelname, void * data)
{
  RemixLADSPA * al = (RemixLADSPA *) data;
  LADSPA_Descriptor * d;
  LADSPA_PortDescriptor pd;
  unsigned long port_i;

  d = al->d;

  /* Connect audio input */
  for (port_i = 0; port_i < d->PortCount; port_i++) {
    pd = d->PortDescriptors[(int)port_i];
    if (LADSPA_IS_AUDIO_INPUT(pd)) {
      d->connect_port (al->handle, port_i, &chunk->data[offset]);
    }
  }

  d->run (al->handle, count);

  return count;
}

static RemixCount
remix_ladspa_0_1 (RemixEnv * env, RemixChunk * chunk, RemixCount offset,
	       RemixCount count, int channelname, void * data)
{
  RemixLADSPA * al = (RemixLADSPA *) data;
  LADSPA_Descriptor * d;
  LADSPA_PortDescriptor pd;
  unsigned long port_i;

  d = al->d;
  
  /* Connect audio output */
  for (port_i = 0; port_i < d->PortCount; port_i++) {
    pd = d->PortDescriptors[(int)port_i];
    if (LADSPA_IS_AUDIO_OUTPUT(pd)) {
      d->connect_port (al->handle, port_i, &chunk->data[offset]);
    }
  }

  d->run (al->handle, count);

  return count;
}

/*
 * remix_ladspa_1_1: An RemixChunkChunkFunc for filtering with a mono LADSPA plugin
 */
static RemixCount
remix_ladspa_1_1 (RemixEnv * env, RemixChunk * src, RemixCount src_offset,
	       RemixChunk * dest, RemixCount dest_offset,
	       RemixCount count, int channelname, void * data)
{
  RemixLADSPA * al = (RemixLADSPA *) data;
  LADSPA_Descriptor * d;
  LADSPA_PortDescriptor pd;
  unsigned long port_i;

  d = al->d;

  if (al == NULL) {
    remix_set_error (env, REMIX_ERROR_INVALID);
    return -1;
  }

  for (port_i = 0; port_i < d->PortCount; port_i++) {
    pd = d->PortDescriptors[(int)port_i];
    if (LADSPA_IS_AUDIO_INPUT(pd)) {
      d->connect_port (al->handle, port_i, &src->data[src_offset]);
    }
    if (LADSPA_IS_AUDIO_OUTPUT(pd)) {
      d->connect_port (al->handle, port_i, &dest->data[dest_offset]);
    }
  }

  d->run (al->handle, count);

  return count;
}


#if 0
static void
ladspa_wrapper_apply_region (RemixEnv * env, gpointer pcmdata, sw_format * format,
			     gint nr_frames,
			     sw_param_set pset, void * custom_data)
{
  lm_custom * lm = (lm_custom *)custom_data;
  const LADSPA_Descriptor * d = lm->d;
  sw_param_spec * param_specs = lm->param_specs;

  LADSPA_Handle * handle;
  LADSPA_Data ** input_buffers, ** output_buffers;
  LADSPA_Data * mono_input_buffer=NULL;
  LADSPA_Data * p;
  LADSPA_Data * control_inputs;
  LADSPA_Data dummy_control_output;
  LADSPA_PortDescriptor pd;
  long length_b;
  unsigned long port_i; /* counter for iterating over ports */
  int i, j, n;

  /* The number of times the plugin will be run; ie. if the number of
   * channels in the input pcmdata is greater than the number of
   * audio ports on the ladspa plugin, the plugin will be run
   * multiple times until enough output channels have been calculated.
   */
  gint iterations;

  /* Enumerate the numbers of each type of port on the ladspa plugin */
  gint
    nr_ci=0, /* control inputs */
    nr_ai=0, /* audio inputs */
    nr_co=0, /* control outputs */
    nr_ao=0; /* audio outputs */

  /* The number of audio channels to be processed */
  gint nr_channels = format->channels;

  /* The number of input and output buffers to use */
  gint nr_i=0, nr_o=0;

  /* Counters for allocating input and output buffers */
  gint ibi=0, obi=0;


  /* instantiate the ladspa plugin */
  handle = d->instantiate (d, (long)format->rate);

  /* Cache how many of each type of port this ladspa plugin has */
  for (port_i=0; port_i < d->PortCount; port_i++) {
    pd = d->PortDescriptors[(int)port_i];
    if (LADSPA_IS_CONTROL_INPUT(pd))
      nr_ci++;
    if (LADSPA_IS_AUDIO_INPUT(pd))
      nr_ai++;
    if (LADSPA_IS_CONTROL_OUTPUT(pd))
      nr_co++;
    if (LADSPA_IS_AUDIO_OUTPUT(pd))
      nr_ao++;
  }

  /* Basic assumption of this wrapper plugin, which was
   * checked above in is_usable(); nb. for future expansion
   * much of this routine is written to accomodate this
   * assumption being incorrect.
   */
  g_assert (nr_ai == nr_ao);

  /* Basic assumption that this plugin has audio output.
   * Also important as we are about to divide by nr_ao.
   */
  g_assert (nr_ao > 0);

  iterations = (gint) ceil(((double)nr_channels) / ((double)nr_ao));

  /* Numbers of input and output buffers: ensure
   * nr_i >= nr_channels && nr_o >= nr_channels
   */
  nr_i = iterations * nr_ai;
  nr_o = iterations * nr_ao;

  if ((nr_channels == 1) && (nr_ai == 1) && (nr_ao >= 1)) {
    /*
     * Processing a mono sample with a mono filter.
     * Attempt to do this in place.
     */

    /* Copy PCM data if this ladspa plugin cannot work inplace */
    if (LADSPA_WRAPPER_IS_INPLACE_BROKEN(d->Properties)) {
      length_b = frames_to_bytes (format, nr_frames);
      mono_input_buffer = g_malloc (length_b);
      input_buffers = &mono_input_buffer;
    } else {
      input_buffers = (LADSPA_Data **)&pcmdata;
    }
    
    output_buffers = (LADSPA_Data **)&pcmdata;

  } else {
    length_b = LADSPA_frames_to_bytes (nr_frames);

    /* Allocate zeroed input buffers; these will remain zeroed
     * if there aren't enough channels in the input pcmdata
     * to use them.
     */
    input_buffers = g_malloc (sizeof(LADSPA_Data *) * nr_i);
    for (i=0; i < nr_i; i++) {
      input_buffers[i] = g_malloc0 (length_b);
    }

    output_buffers = g_malloc(sizeof(LADSPA_Data *) * nr_o);

    /* Create separate output buffers if this ladspa plugin cannot
     * work inplace */
    if (LADSPA_WRAPPER_IS_INPLACE_BROKEN(d->Properties)) {
      for (i=0; i < nr_o; i++) {
	output_buffers[i] = g_malloc (length_b);
      }
    } else {
      /* Re-use the input buffers, directly mapping them to
       * corresponding output buffers
       */
      for (i=0; i < MIN(nr_i, nr_o); i++) {
	output_buffers[i] = input_buffers[i];
      }
      /* Create some extra output buffers if nr_o > nr_i */
      for (; i < nr_o; i++) {
	output_buffers[i] = g_malloc (length_b);
      }
    }
  }

  /* Copy data into input buffers */
  if (nr_channels == 1) {
    if (!LADSPA_WRAPPER_IS_INPLACE_BROKEN(d->Properties)) {
      length_b = frames_to_bytes (format, nr_frames);
      memcpy (input_buffers[0], pcmdata, length_b);
    } /* else we're processing in-place, so we haven't needed to set
       * up a separate input buffer; input_buffers[0] actually
       * points to pcmdata hence we don't do any copying here.
       */
  } else {
    /* de-interleave multichannel data */

    p = (LADSPA_Data *)pcmdata;

    for (n=0; n < nr_channels; n++) {
      for (i=0; i < nr_frames; i++) {
	input_buffers[n][i] = *p++;
      }
    }
  }

  /* connect control ports */
  control_inputs = g_malloc (nr_ci * sizeof(LADSPA_Data));
  j=0;
  for (port_i=0; port_i < d->PortCount; port_i++) {
    pd = d->PortDescriptors[(int)port_i];
    if (LADSPA_IS_CONTROL_INPUT(pd)) {
      /* do something with pset! */
      switch (param_specs[j].type) {
      case SWEEP_TYPE_BOOL:
	/* from ladspa.h:
	 * Data less than or equal to zero should be considered
	 * `off' or `false,'
	 * and data above zero should be considered `on' or `true.'
	 */
	control_inputs[j] = pset[j].b ? 1.0 : 0.0;
	break;
      case SWEEP_TYPE_INT:
	control_inputs[j] = (LADSPA_Data)pset[j].i;
	break;
      case SWEEP_TYPE_FLOAT:
	control_inputs[j] = pset[j].f;
	break;
      default:
	/* This plugin should produce no other types */
	g_assert_not_reached ();
	break;
      }
      d->connect_port (handle, port_i, &control_inputs[j]);
      j++;
    }
    if (LADSPA_IS_CONTROL_OUTPUT(pd)) {
      d->connect_port (handle, port_i, &dummy_control_output);
    }
  }

  /* run the plugin as many times as necessary */
  while (iterations--) {

    /* connect input and output audio buffers to the
     * audio ports of the ladspa plugin */
    for (port_i=0; port_i < d->PortCount; port_i++) {
      pd = d->PortDescriptors[(int)port_i];
      if (LADSPA_IS_AUDIO_INPUT(pd)) {
	d->connect_port (handle, port_i, input_buffers[ibi++]);
      }
      if (LADSPA_IS_AUDIO_OUTPUT(pd)) {
	d->connect_port (handle, port_i, output_buffers[obi++]);
      }
    }

    /* activate the ladspa plugin */
    if (d->activate)
      d->activate (handle);

    /* run the ladspa plugin */
    d->run (handle, nr_frames);

    /* deactivate the ladspa plugin */
    if (d->deactivate)
      d->deactivate (handle);
  }

  /* re-interleave data */
  if (nr_channels > 1) {
    p = (LADSPA_Data *)pcmdata;

    for (n=0; n < nr_channels; n++) {
      for (i=0; i < nr_frames; i++) {
	*p++ = output_buffers[n][i];
      }
    }
  }

  /* let the ladspa plugin clean up after itself */
  if (d->cleanup)
    d->cleanup (handle);

  /* free the input and output buffers */
  if (control_inputs) g_free (control_inputs);

  if ((nr_channels == 1) && (nr_ai == 1) && (nr_ao >= 1)) {
    if (LADSPA_WRAPPER_IS_INPLACE_BROKEN(d->Properties)) {
      g_free (mono_input_buffer);
    }
  } else {

    /* free the output buffers */
    for (i=0; i < nr_o; i++) {
      g_free (output_buffers[i]);
    }
    g_free (output_buffers);

    /* free the input buffers, if we created some */
    if (LADSPA_WRAPPER_IS_INPLACE_BROKEN(d->Properties)) {
      for (i=0; i < nr_i; i++) {
	g_free (input_buffers[i]);
      }
    } else {
      /* inplace worked, but if (nr_i > nr_o), then
       * we still need to free the last input buffers
       */
      for (i=nr_o; i < nr_i; i++) {
	g_free (input_buffers[i]);
      }
    }
    g_free (input_buffers);  
  }
}
#endif

static RemixBase *
remix_ladspa_connect_control_inputs (RemixEnv * env, RemixBase * base)
{
  RemixLADSPA * al = (RemixLADSPA *) remix_base_get_instance_data (env, base);
  LADSPA_Descriptor * d;
  LADSPA_PortDescriptor pd;
  RemixParameter parameter;
  RemixParameterType type;
  int j;
  unsigned long port_i;

  d = al->d;

  if (d == NULL) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return RemixNone;
  }

  j=0;
  for (port_i=0; port_i < d->PortCount; port_i++) {
    pd = d->PortDescriptors[(int)port_i];
    if (LADSPA_IS_CONTROL_INPUT(pd)) {
      type = remix_get_parameter_type (env, base, j);
      parameter = remix_get_parameter (env, base, j);
      switch (type) {
      case REMIX_TYPE_BOOL:
	/* from ladspa.h:
	 * Data less than or equal to zero should be considered
	 * `off' or `false,'
	 * and data above zero should be considered `on' or `true.'
	 */
	al->control_inputs[j] = parameter.s_bool ? 1.0 : 0.0;
	break;
      case REMIX_TYPE_INT:
	al->control_inputs[j] = (LADSPA_Data)parameter.s_int;
	break;
      case REMIX_TYPE_FLOAT:
	al->control_inputs[j] = parameter.s_float;
	break;
      default:
	/* This plugin should produce no other types */
	break;
      }
      d->connect_port (al->handle, port_i, &al->control_inputs[j]);
      j++;
    }
    if (LADSPA_IS_CONTROL_OUTPUT(pd)) {
      d->connect_port (al->handle, port_i, &dummy_control_output);
    }
  }

  return base;
}

static RemixCount
remix_ladspa_1_0_process (RemixEnv * env, RemixBase * base, RemixCount count,
		       RemixStream * input, RemixStream * output)
{
  RemixLADSPA * al = remix_base_get_instance_data (env, base);
  remix_ladspa_connect_control_inputs (env, base);
  return remix_stream_chunkfuncify (env, input, count, remix_ladspa_1_0, al);
}

static RemixCount
remix_ladspa_0_1_process (RemixEnv * env, RemixBase * base, RemixCount count,
		       RemixStream * input, RemixStream * output)
{
  RemixLADSPA * al = remix_base_get_instance_data (env, base);
  remix_ladspa_connect_control_inputs (env, base);
  return remix_stream_chunkfuncify (env, output, count, remix_ladspa_0_1, al);
}

static RemixCount
remix_ladspa_1_1_process (RemixEnv * env, RemixBase * base, RemixCount count,
		       RemixStream * input, RemixStream * output)
{
  RemixLADSPA * al = remix_base_get_instance_data (env, base);
  remix_ladspa_connect_control_inputs (env, base);
  return remix_stream_chunkchunkfuncify (env, input, output, count,
				      remix_ladspa_1_1, al);
}

static RemixCount
remix_ladspa_process (RemixEnv * env, RemixBase * base, RemixCount count,
		   RemixStream * input, RemixStream * output)
{
  RemixPlugin * plugin = remix_base_get_plugin (env, base);
  RemixLADSPA * al;
  LADSPA_PortDescriptor pd;
  unsigned long port_i; /* counter for iterating over ports */

  /* Enumerate the numbers of each type of port on the ladspa plugin */
  int
    nr_ci=0, /* control inputs */
    nr_ai=0, /* audio inputs */
    nr_co=0, /* control outputs */
    nr_ao=0; /* audio outputs */

  if (plugin == RemixNone) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return -1;
  }

  remix_ladspa_connect_control_inputs (env, base);

  al = remix_base_get_instance_data (env, base);

  /* Cache how many of each type of port this ladspa plugin has */
  for (port_i=0; port_i < al->d->PortCount; port_i++) {
    pd = al->d->PortDescriptors[(int)port_i];
    if (LADSPA_IS_CONTROL_INPUT(pd))
      nr_ci++;
    if (LADSPA_IS_AUDIO_INPUT(pd))
      nr_ai++;
    if (LADSPA_IS_CONTROL_OUTPUT(pd))
      nr_co++;
    if (LADSPA_IS_AUDIO_OUTPUT(pd))
      nr_ao++;
  }

  if (nr_ai == 1 && nr_ao == 1) {
    return remix_stream_chunkchunkfuncify (env, input, output, count,
					remix_ladspa_1_1, al);
  } else if (nr_ai == 1 && nr_ao == 0) {
    return remix_stream_chunkfuncify (env, input, count, remix_ladspa_1_0, al);
  } else if (nr_ai == 0 && nr_ao == 1) {
    return remix_stream_chunkfuncify (env, output, count, remix_ladspa_0_1, al);
  } else {
    remix_set_error (env, REMIX_ERROR_INVALID);
    return -1;
  }
}

static RemixCount
remix_ladspa_length (RemixEnv * env, RemixBase * base)
{
  return REMIX_COUNT_INFINITE;
}

static struct _RemixMethods _remix_ladspa_1_0_methods = {
  remix_ladspa_clone,
  remix_ladspa_destroy,
  remix_ladspa_ready, /* ready */
  remix_ladspa_prepare, /* prepare */
  remix_ladspa_1_0_process,
  remix_ladspa_length,
  NULL, /* seek */
  NULL, /* flush */
};

static struct _RemixMethods _remix_ladspa_0_1_methods = {
  remix_ladspa_clone,
  remix_ladspa_destroy,
  remix_ladspa_ready, /* ready */
  remix_ladspa_prepare, /* prepare */
  remix_ladspa_0_1_process,
  remix_ladspa_length,
  NULL, /* seek */
  NULL, /* flush */
};

static struct _RemixMethods _remix_ladspa_1_1_methods = {
  remix_ladspa_clone,
  remix_ladspa_destroy,
  remix_ladspa_ready, /* ready */
  remix_ladspa_prepare, /* prepare */
  remix_ladspa_1_1_process,
  remix_ladspa_length,
  NULL, /* seek */
  NULL, /* flush */
};


static struct _RemixMethods _remix_ladspa_methods = {
  remix_ladspa_clone,
  remix_ladspa_destroy,
  remix_ladspa_ready, /* ready */
  remix_ladspa_prepare, /* prepare */
  remix_ladspa_process,
  remix_ladspa_length,
  NULL, /* seek */
  NULL, /* flush */
};

static RemixBase *
remix_ladspa_optimise (RemixEnv * env, RemixBase * base)
{
  RemixLADSPA * al = remix_base_get_instance_data (env, base);
  LADSPA_Descriptor * d;
  LADSPA_PortDescriptor pd;
  unsigned long port_i; /* counter for iterating over ports */

  /* Enumerate the numbers of each type of port on the ladspa plugin */
  int
    nr_ci=0, /* control inputs */
    nr_ai=0, /* audio inputs */
    nr_co=0, /* control outputs */
    nr_ao=0; /* audio outputs */

  if (al == NULL) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return RemixNone;
  }

  d = al->d;

  if (d == NULL) {
    remix_set_error (env, REMIX_ERROR_NOENTITY);
    return RemixNone;
  }

  /* Cache how many of each type of port this ladspa plugin has */
  for (port_i=0; port_i < d->PortCount; port_i++) {
    pd = d->PortDescriptors[(int)port_i];
    if (LADSPA_IS_CONTROL_INPUT(pd))
      nr_ci++;
    if (LADSPA_IS_AUDIO_INPUT(pd))
      nr_ai++;
    if (LADSPA_IS_CONTROL_OUTPUT(pd))
      nr_co++;
    if (LADSPA_IS_AUDIO_OUTPUT(pd))
      nr_ao++;
  }

  al->control_inputs = malloc (nr_ci * sizeof (LADSPA_Data));

  if (nr_ai == 1 && nr_ao == 1) {
    remix_base_set_methods (env, base, &_remix_ladspa_1_1_methods);
  } else if (nr_ai == 1 && nr_ao == 0) {
    remix_base_set_methods (env, base, &_remix_ladspa_1_0_methods);
  } else if (nr_ai == 0 && nr_ao == 1) {
    remix_base_set_methods (env, base, &_remix_ladspa_0_1_methods);
  } else { 
    remix_base_set_methods (env, base, &_remix_ladspa_methods);
  }

  return base;
}


/*
 * ladspa_wrapper_load_plugins (dir, name, gl)
 *
 * form RemixPlugins to describe the ladspa plugin functions that
 * are in the shared library file "dir/name"
 */
static CDList *
ladspa_wrapper_load_plugins (RemixEnv * env, char * dir, char * name)
{
#define PATH_LEN 256
  char path[PATH_LEN];
  void * module;
  LADSPA_Descriptor_Function desc_func;
  const LADSPA_Descriptor * d;
  LADSPA_PortDescriptor pd;
  int i, j, k;
  int valid_mask;
  RemixPlugin * plugin;
  RemixMetaText * mt;
  RemixParameterScheme * scheme;
  CDList * plugins = CD_EMPTY_LIST;
#define BUF_LEN 256
  static char buf[BUF_LEN];

  snprintf (path, PATH_LEN, "%s/%s", dir, name);

  module = dlopen (path, RTLD_NOW);
  if (!module) return CD_EMPTY_LIST;

  if ((desc_func = dlsym (module, "ladspa_descriptor"))) {
    for (i=0; (d = desc_func (i)) != NULL; i++) {

      if (!is_usable(d))
	continue;

      remix_dprintf ("[ladspa_wrapper_load_plugins] adding %s [%lu] by %s\n",
		  d->Name, d->UniqueID, d->Maker);

      plugin = malloc (sizeof (*plugin));
      
      mt = remix_meta_text_new (env);

      snprintf (buf, BUF_LEN, "ladspa::%lu", d->UniqueID);
      remix_meta_text_set_identifier (env, mt, strdup (buf));

      snprintf (buf, BUF_LEN, "Miscellaneous::%s", d->Name);
      remix_meta_text_set_category (env, mt, strdup (buf));

      remix_meta_text_set_copyright (env, mt, (char *)d->Copyright);
      remix_meta_text_add_author (env, mt, (char *)d->Maker, NULL);

      plugin->metatext = mt;

      plugin->init_scheme = CD_EMPTY_SET;
      plugin->process_scheme = CD_EMPTY_SET;

      k=0;
      for (j=0; j < d->PortCount; j++) {
	pd = d->PortDescriptors[j];
	if (LADSPA_IS_CONTROL_INPUT(pd)) {
	  scheme = malloc (sizeof (*scheme));

	  scheme->name = (char *)d->PortNames[j];
	  scheme->description = (char *)d->PortNames[j];
	  scheme->type = convert_type (d->PortRangeHints[j].HintDescriptor);
	  valid_mask = get_valid_mask (d->PortRangeHints[j].HintDescriptor);
	  if (valid_mask == 0) {
	    scheme->constraint_type = REMIX_CONSTRAINT_TYPE_NONE;
	  } else {
	    scheme->constraint_type = REMIX_CONSTRAINT_TYPE_RANGE;
	    scheme->constraint.range =
	      convert_constraint (&d->PortRangeHints[j]);
	  }
	  plugin->process_scheme = cd_set_insert (env, plugin->process_scheme,
						  k, CD_POINTER(scheme));
	  k++;
	}
      }

      plugin->init = remix_ladspa_init;

      plugin->plugin_data = (void *)d;

      plugins = cd_list_append (env, plugins, CD_POINTER(plugin));
    }
  }

  return plugins;

#undef BUF_LEN
}

/*
 * ladspa_wrapper_load_dir (dir, gl)
 *
 * scan a directory "dirname" for LADSPA plugins, and attempt to load
 * each of them.
 */
static CDList *
ladspa_wrapper_load_dir (RemixEnv * env, char * dirname)
{
  DIR * dir;
  struct dirent * dirent;
  CDList * plugins = CD_EMPTY_LIST, * l;

  if (!dirname) return plugins;

  dir = opendir (dirname);
  if (!dir) {
    return plugins;
  }

  while ((dirent = readdir (dir)) != NULL) {
    l = ladspa_wrapper_load_plugins (env, dirname, dirent->d_name);
    plugins = cd_list_join (env, plugins, l);
  }

  return plugins;
}

CDList *
remix_load (RemixEnv * env)
{
  CDList * plugins = CD_EMPTY_LIST, * l;
  char * ladspa_path=NULL;
  char * next_sep=NULL;
  char * saved_lp=NULL;

  /* If this ladspa_wrapper module has already been initialised, don't
   * initialise again until cleaned up.
   */
  if (ladspa_wrapper_initialised)
    return CD_EMPTY_LIST;

  ladspa_path = getenv ("LADSPA_PATH");
  if (!ladspa_path)
    ladspa_path = saved_lp = strdup(default_ladspa_path);

  do {
    next_sep = strchr (ladspa_path, ':');
    if (next_sep != NULL) *next_sep = '\0';
    
    l = ladspa_wrapper_load_dir (env, ladspa_path);
    plugins = cd_list_join (env, plugins, l);

    if (next_sep != NULL) ladspa_path = ++next_sep;

  } while ((next_sep != NULL) && (*next_sep != '\0'));

  ladspa_wrapper_initialised = TRUE;

  /* free string if dup'd for ladspa_path */
  if (saved_lp != NULL) free(saved_lp);

  return plugins;
}

void
remix_unload (RemixEnv * env)
{
  CDList * l;

  if (!ladspa_wrapper_initialised) return;

  for (l = modules_list; l; l = l->next) {
    dlclose(l->data.s_pointer);
  }
}

