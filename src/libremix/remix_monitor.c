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
 * RemixMonitor: device output
 *
 * Conrad Parker <Conrad.Parker@CSIRO.AU>, August 2001
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include <config.h>

#if (HAVE_ALSA == 1)
#define ALSA_PCM_OLD_HW_PARAMS_API
#include <alsa/asoundlib.h>
#else
typedef void snd_pcm_t;
#endif

#define __REMIX__
#include "remix.h"

#define DEFAULT_FORMAT AFMT_S16_LE
#define DEFAULT_STEREO 1
#define DEFAULT_FREQUENCY 44100
#define DEFAULT_NUMFRAGS 4
#define DEFAULT_FRAGSIZE 10

/* Must be either 1 or 0. */
#define DEBUG_FILE	0

#if (DEBUG_FILE == 1)
/* make sure this file exists; we're not using O_CREAT! */
#define FILENAME "/tmp/env.out"
#else
#define FILENAME "/dev/dsp"
#endif

/* Optimisation dependencies: none */
static RemixMonitor * remix_monitor_optimise (RemixEnv * env, RemixMonitor * monitor);

static RemixBase *
alsa_reset_device (RemixEnv * env, RemixBase * base, snd_pcm_t * alsa_dev,
		   int channels, int samplerate);

static snd_pcm_t * alsa_open_device (RemixEnv * env, RemixBase * base);

static RemixCount
alsa_write_float (RemixEnv * env, snd_pcm_t * alsa_dev, RemixPCM *data,
		  RemixCount playcount);

static void alsa_close_device (RemixMonitor * monitor);


static RemixBase *
remix_monitor_reset_device (RemixEnv * env, RemixBase * base)
{
  RemixMonitor * monitor = (RemixMonitor *)base;
  CDSet * channels = remix_get_channels (env);
  RemixCount nr_channels;
  int fragmentsize;

  nr_channels = cd_set_size (env, channels);

  if (nr_channels == 1)
    monitor->stereo = 0;
  else if (nr_channels > 1)
    monitor->stereo = 1;

  monitor->mask = 0;
  monitor->format = DEFAULT_FORMAT;

  monitor->frequency = remix_get_samplerate (env);
  monitor->numfrags = DEFAULT_NUMFRAGS;
  monitor->fragsize = DEFAULT_FRAGSIZE;

  if (DEBUG_FILE == 1) {
    monitor->format = AFMT_S16_LE;
    return base;
  }

  if (monitor->alsa_dev != NULL) {
    alsa_reset_device (env, base, monitor->alsa_dev, nr_channels, monitor->frequency);
	return base;
  }

  if (ioctl (monitor->dev_dsp_fd, SNDCTL_DSP_GETFMTS, &monitor->mask) == -1) {
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return RemixNone;
  }

  if (monitor->mask & AFMT_U8) {
    monitor->format = AFMT_U8;
  }
  if (monitor->mask & AFMT_U16_LE) {
    monitor->format = AFMT_U16_LE;
  }
  if (monitor->mask & AFMT_S16_LE) {
    monitor->format = AFMT_S16_LE;
  }
  if (monitor->mask & AFMT_U16_BE) {
    monitor->format = AFMT_U16_BE;
  }
  if (monitor->mask & AFMT_S16_BE) {
    monitor->format = AFMT_S16_BE;
  }
  if (monitor->mask & AFMT_S8) {
    monitor->format = AFMT_S8;
  }
  if (monitor->mask & AFMT_S16_LE) {
    monitor->format = AFMT_S16_LE;
  }

  if (ioctl(monitor->dev_dsp_fd, SNDCTL_DSP_SETFMT, &monitor->format) == -1) {
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return RemixNone;
  }

  if (ioctl(monitor->dev_dsp_fd, SNDCTL_DSP_STEREO, &(monitor->stereo)) == -1) {
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return RemixNone;
  }

  if (ioctl(monitor->dev_dsp_fd, SNDCTL_DSP_SPEED, &(monitor->frequency)) == -1) {
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return RemixNone;
  }

  fragmentsize = (monitor->numfrags << 16) | monitor->fragsize;
  if (ioctl(monitor->dev_dsp_fd, SNDCTL_DSP_SETFRAGMENT, &fragmentsize) == -1) {
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return RemixNone;
  }

  return base;
}

static RemixBase *
remix_monitor_init (RemixEnv * env, RemixBase * base)
{
  RemixMonitor * monitor = (RemixMonitor *)base;
  
  monitor->dev_dsp_fd = -1;
  monitor->alsa_dev = NULL;
  
  if (HAVE_ALSA == 1) {
	monitor->alsa_dev = alsa_open_device (env, base);
  }
  
  if (monitor->alsa_dev == NULL) {
    monitor->dev_dsp_fd = open (FILENAME, O_WRONLY, 0);
    if (monitor->dev_dsp_fd == -1) {
      printf ("Couldn't open any output device.\n");
      remix_set_error (env, REMIX_ERROR_SYSTEM);
      return RemixNone;
    }
  }

  remix_monitor_reset_device (env, base);

  remix_monitor_optimise (env, monitor);

  return (RemixBase *)monitor;
}

RemixMonitor *
remix_monitor_new (RemixEnv * env)
{
  RemixBase * monitor = remix_base_new_subclass (env, sizeof (struct _RemixMonitor));

  remix_monitor_init (env, monitor);
  return (RemixMonitor *)monitor;
}

static RemixBase *
remix_monitor_clone (RemixEnv * env, RemixBase * base)
{
  RemixMonitor * new_monitor = remix_monitor_new (env);

  remix_monitor_optimise (env, new_monitor);
  return (RemixBase *)new_monitor;
}

static int
remix_monitor_destroy (RemixEnv * env, RemixBase * base)
{
  RemixMonitor * monitor = (RemixMonitor *)base;
  
  if (monitor->alsa_dev != NULL) {
    alsa_close_device (monitor);
  } else if (monitor->dev_dsp_fd != -1) {
    close (monitor->dev_dsp_fd);
  }
  remix_free (monitor);
  return 0;
}

static int
remix_monitor_ready (RemixEnv * env, RemixBase * base)
{
  RemixMonitor * monitor = (RemixMonitor *) base;
  CDSet * channels = remix_get_channels (env);
  int samplerate = (int) remix_get_samplerate (env);
  RemixCount nr_channels;

  nr_channels = cd_set_size (env, channels);

  return (samplerate == monitor->frequency &&
	  ((nr_channels == 1 && monitor->stereo == 0) ||
	   (nr_channels > 1 && monitor->stereo == 1)));
}

static RemixBase *
remix_monitor_prepare (RemixEnv * env, RemixBase * base)
{
  remix_monitor_reset_device (env, base);
  return base;
}

static RemixCount
remix_monitor_write_short (RemixEnv * env, RemixMonitor * monitor, RemixCount count)
{
  static struct timeval tv_instant = {0, 0};

  RemixCount n = 0;
  fd_set fds;

  if (monitor->alsa_dev != NULL) {
    printf ("1 ######## Should not be here if using ALSA.\n");
	return count;
  }

  if (!(monitor->format & AFMT_S16_LE)) {
    printf ("###### device cannot play AFMT_S16_LE nicely\n");
    return count;
  }

  FD_ZERO (&fds);
  FD_SET (monitor->dev_dsp_fd, &fds);

  if ((select (monitor->dev_dsp_fd + 1, NULL, &fds, NULL, &tv_instant) == 0));
#if 0
  {
    printf ("select error\n");
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return -1;
  }
#endif
  
  n = write (monitor->dev_dsp_fd, monitor->playbuffer, count * sizeof(short));
  if (n == -1) {
    printf ("####### system error writing to fd %d #######\n",
	    monitor->dev_dsp_fd);
    
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return -1;
  }

  return n;
}

static RemixCount
remix_monitor_playbuffer (RemixEnv * env, RemixMonitor * monitor, RemixPCM * data,
			 RemixCount count)
{
  RemixCount i;
  RemixPCM value;
  const RemixPCM max_value = (RemixPCM)SHRT_MAX / 2;

  if (monitor->alsa_dev != NULL) {
    /* Assuming here that we have two channels. */
    i = alsa_write_float (env, monitor->alsa_dev, data, count / 2) * 2;
    
    return i;
  }

  for (i = 0; i < count; i++) {
    value = *data++ * max_value;
    monitor->playbuffer[i] = (short)value;
  }

  count = remix_monitor_write_short (env, monitor, count);
  
  return count;
}

/* An RemixChunkFunc for making noise */
static RemixCount
remix_monitor_chunk (RemixEnv * env, RemixChunk * chunk, RemixCount offset,
		    RemixCount count, int channelname, void * data)
{
  RemixMonitor * monitor = (RemixMonitor *)data;
  RemixCount remaining = count, written = 0, n, playcount;
  RemixPCM * d;
 
  if (monitor->alsa_dev != NULL) {
    while (remaining > 0) {
      playcount = MIN (remaining, REMIX_MONITOR_BUFFERLEN);
      d = &chunk->data[offset];
      n = alsa_write_float (env, monitor->alsa_dev, d, playcount);

      if (n == -1) {
        return -1;
      }

      offset += n;
      written += n;
      remaining -= n;      
    }
    return written;
  }

  if (monitor->dev_dsp_fd == -1) {
    remix_dprintf ("[remix_monitor_chunk] no file\n");
    remix_set_error (env, REMIX_ERROR_NOENTITY); /* XXX: different error ? */
    return -1;
  }

  remix_dprintf ("[remix_monitor_chunk] (%p [chunk %p], +%ld), @ %ld\n",
		monitor, chunk, count, offset);

  while (remaining > 0) {
    playcount = MIN (remaining, REMIX_MONITOR_BUFFERLEN);
    d = &chunk->data[offset];
    n = remix_monitor_playbuffer (env, monitor, d, playcount);

    if (n == -1) {
      return -1;
    } else {
      n /= sizeof (short);
    }

    offset += n;
    written += n;
    remaining -= n;      
  }

  return written;
}

static RemixCount
remix_monitor_process (RemixEnv * env, RemixBase * base, RemixCount count,
		      RemixStream * input, RemixStream * output)
{
  RemixMonitor * monitor = (RemixMonitor *)base;
  RemixCount nr_channels = remix_stream_nr_channels (env, input);
  RemixCount remaining = count, processed = 0, n, nn;

  if (nr_channels == 1 && monitor->stereo == 0) { /* MONO */
    return remix_stream_chunkfuncify (env, input, count,
				     remix_monitor_chunk, monitor);
  } else if (nr_channels == 2 && monitor->stereo == 1) { /* STEREO */

    while (remaining > 0) {
      n = MIN (remaining, REMIX_MONITOR_BUFFERLEN/2);
      n = remix_stream_interleave_2 (env, input,
				    REMIX_CHANNEL_LEFT, REMIX_CHANNEL_RIGHT,
				    monitor->databuffer, n);
      nn = 2 * n;
      nn = remix_monitor_playbuffer (env, monitor, monitor->databuffer, nn);

      processed += n;
      remaining -= n;
    }
    return processed;
  } else {
    printf ("[remix_monitor_process] unsupported stream/output channel\n");
    printf ("combination %ld / %d\n", nr_channels, monitor->stereo ? 2 : 1);
    return -1;
  }
}

static RemixCount
remix_monitor_length (RemixEnv * env, RemixBase * base)
{
  return REMIX_COUNT_INFINITE;
}

static RemixCount
remix_monitor_seek (RemixEnv * env, RemixBase * base, RemixCount count)
{
  return count;
}

static int
remix_monitor_flush (RemixEnv * env, RemixBase * base)
{
  RemixMonitor * monitor = (RemixMonitor *)base;

  if (DEBUG_FILE == 1)
  	return 0;

  if (ioctl(monitor->dev_dsp_fd, SNDCTL_DSP_POST, NULL) == -1) {
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return -1;
  }

  return 0;
}

static struct _RemixMethods _remix_monitor_methods = {
  remix_monitor_clone,
  remix_monitor_destroy,
  remix_monitor_ready,
  remix_monitor_prepare,
  remix_monitor_process,
  remix_monitor_length,
  remix_monitor_seek,
  remix_monitor_flush,
};

static RemixMonitor *
remix_monitor_optimise (RemixEnv * env, RemixMonitor * monitor)
{
  _remix_set_methods (env, (RemixBase *)monitor, &_remix_monitor_methods);
  return monitor;
}

#if (HAVE_ALSA == 0)

static snd_pcm_t *
alsa_open_device (RemixEnv * env, RemixBase * base)
{	
  env = env;
  base = base;
  return NULL;
}

static RemixBase *
alsa_reset_device (RemixEnv * env, RemixBase * base, snd_pcm_t * alsa_dev, int channels, int samplerate)
{	
  env = env;
  alsa_dev = alsa_dev;
  channels = channels;
  samplerate = samplerate;
  return base;
}

static RemixCount
alsa_write_float (RemixEnv * env, snd_pcm_t * alsa_dev, RemixPCM *data,
		  RemixCount playcount)
{
  env = env;
  alsa_dev = alsa_dev;
  data = data;
  return playcount;
}

static void
alsa_close_device (RemixMonitor * monitor)
{
  monitor = monitor;
}

#else

static snd_pcm_t *
alsa_open_device (RemixEnv * env, RemixBase * base)
{
  snd_pcm_t *alsa_device = NULL;
  char	*device_name = "default";
  int	error;
  
  if ((error = snd_pcm_open (&alsa_device, device_name, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    return NULL;
  }

  return alsa_device;  
}

static RemixBase *
alsa_reset_device (RemixEnv * env, RemixBase * base, snd_pcm_t * alsa_dev,
		   int channels, int samplerate)
{
  snd_pcm_hw_params_t *hw_params;
  int periods;
  int error;
	
  if ((error = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return RemixNone;
  }

  if ((error = snd_pcm_hw_params_any (alsa_dev, hw_params)) < 0) {
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return RemixNone;
  }
  
  if ((error = snd_pcm_hw_params_set_access (alsa_dev, hw_params,
					     SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return RemixNone;
  }

  if ((error = snd_pcm_hw_params_set_format (alsa_dev, hw_params,
					     SND_PCM_FORMAT_FLOAT)) < 0) {
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return RemixNone;
  }
  
  if ((error = snd_pcm_hw_params_set_rate_near (alsa_dev, hw_params,
						samplerate, 0)) < 0) {
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return RemixNone;
  }
  
  if ((error = snd_pcm_hw_params_set_channels (alsa_dev, hw_params, channels)) < 0) {
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return RemixNone;
  }

  if ((error = snd_pcm_hw_params_set_period_size (alsa_dev, hw_params,
						  REMIX_MONITOR_BUFFERLEN/channels, 0)) < 0) {
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return RemixNone;
  }

  periods = MIN (4, snd_pcm_hw_params_get_periods_max (hw_params, 0));
  periods =
    MAX (periods, snd_pcm_hw_params_get_periods_min (hw_params, 0));

  if ((error = snd_pcm_hw_params_set_periods (alsa_dev, hw_params,
					      periods, 0)) < 0) {
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return RemixNone;
  }

  if ((error = snd_pcm_hw_params (alsa_dev, hw_params)) < 0) {
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return RemixNone;
  }

  snd_pcm_hw_params_free (hw_params);

  if ((error = snd_pcm_prepare (alsa_dev)) < 0) {
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return RemixNone;
  }

  return base;
}

static RemixCount
alsa_write_float (RemixEnv * env, snd_pcm_t * alsa_dev, RemixPCM *data,
		  RemixCount playcount /* frames */)
{
  snd_pcm_status_t * status;
  int err;

  /*printf ("want to write %ld thingies\n", playcount);*/

  if (snd_pcm_wait (alsa_dev, 1000) < 0) {
    printf ("snd_pcm_wait error\n");
    remix_set_error (env, REMIX_ERROR_SYSTEM);
    return -1;
  }

  err = snd_pcm_writei (alsa_dev, data, playcount);

  if (err == -EPIPE) {
    snd_pcm_status_alloca(&status);
    if ((err = snd_pcm_status(alsa_dev, status))<0) {
      fprintf(stderr, "Remix: alsa_write: xrun. can't determine length\n");
    } else {
      if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
        struct timeval now, diff, tstamp;
        gettimeofday(&now, 0);
        snd_pcm_status_get_trigger_tstamp(status, &tstamp);
        timersub(&now, &tstamp, &diff);
        fprintf(stderr, "Remix: alsa_write: xrun of at least %.3f msecs. "
	    "resetting stream\n", diff.tv_sec * 1000 + diff.tv_usec / 1000.0);
      } else {
        fprintf(stderr, "Remix: alsa_write: xrun. can't determine length\n");
      }
    }  
    snd_pcm_prepare(alsa_dev);
    err = snd_pcm_writei(alsa_dev, data, playcount);
    if (err != playcount) {
      fprintf(stderr, "Remix: alsa_write: %s\n", snd_strerror(err));
      return 0;
    } else if (err < 0) {
      fprintf(stderr, "Remix: alsa_write: %s\n", snd_strerror(err));
      return 0;
    }
  }

  return err;
}

static void
alsa_close_device (RemixMonitor * monitor)
{
  snd_pcm_close (monitor->alsa_dev);
  monitor->alsa_dev = NULL;
}
#endif
