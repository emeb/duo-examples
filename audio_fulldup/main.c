/*
 * main.c - long delay based on full duplex audio in/out with low-level ALSA lib
 * 08-20-20 E. Brombaugh
 */

#if 1
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include "main.h"
#include "audio.h"

/* version */
const char *swVersionStr = "V0.1";

/* build time */
const char *bdate = __DATE__;
const char *btime = __TIME__;

/* constants */
#define NUM_RATES 5
const int legal_rates[NUM_RATES] = {32000, 44100, 48000, 88200, 96000};

/* state */
char				*snd_device_in = "hw:0,0";
char 				*snd_device_out = "hw:0,1";
snd_pcm_t			*playback_handle;
snd_pcm_t			*capture_handle;
snd_mixer_t			*mixer_handle;
snd_mixer_selem_id_t *sid;
snd_mixer_elem_t	*elem;
int					nchannels = 2;
int					buffer_size = 4096;
int					sample_rate = 96000;
int 				bits = 16;
int                 dlytime = 1;
int 				err;
int					exit_program = 0;
long				play_vol = 80;
char				*rdbuf;
unsigned int		fragments = 2;
int					frame_size;
snd_pcm_uframes_t   frames, inframes, outframes;

/*
 * set up an audio device
 */
int configure_alsa_audio(snd_pcm_t *device, int channels)
{
	snd_pcm_hw_params_t *hw_params;
	int                 err;
	unsigned int		tmp;

	/* allocate memory for hardware parameter structure */ 
	if((err = snd_pcm_hw_params_malloc(&hw_params)) < 0)
	{
		fprintf (stderr, "cannot allocate parameter structure (%s)\n",
			snd_strerror(err));
		return 1;
	}
	
	/* fill structure from current audio parameters */
	if((err = snd_pcm_hw_params_any(device, hw_params)) < 0)
	{
		fprintf (stderr, "cannot initialize parameter structure (%s)\n",
			snd_strerror(err));
		return 1;
	}

	/* set access type, sample rate, sample format, channels */
	if((err = snd_pcm_hw_params_set_access(device, hw_params,
		SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
	{
		fprintf (stderr, "cannot set access type: %s\n",
			snd_strerror(err));
		return 1;
	}
	
	// bits = 16
	if((err = snd_pcm_hw_params_set_format(device, hw_params,
		SND_PCM_FORMAT_S16_LE)) < 0)
	{
		fprintf (stderr, "cannot set sample format: %s\n",
			snd_strerror(err));
		return 1;
	}
	
	tmp = sample_rate;    
	if((err = snd_pcm_hw_params_set_rate_near(device, hw_params,
		&tmp, 0)) < 0)
	{
		fprintf (stderr, "cannot set sample rate: %s\n",
			snd_strerror(err));
		return 1;
	}
	
	if(tmp != sample_rate)
	{
		fprintf(stderr, "Could not set requested sample rate, %d != %d\n",
			sample_rate, tmp);
		sample_rate = tmp;
	}
	
	if((err = snd_pcm_hw_params_set_channels(device, hw_params, channels)) < 0)
	{
		fprintf (stderr, "cannot set channel count: %s\n",
			snd_strerror(err));
		return 1;
	}
	
	if((err = snd_pcm_hw_params_set_periods_near(device, hw_params,
		&fragments, 0)) < 0)
	{
		fprintf(stderr, "Error setting # fragments to %d: %s\n", fragments,
			snd_strerror(err));
		return 1;
	}

	frame_size = channels * (bits / 8);
	frames = buffer_size / frame_size * fragments;
	if((err = snd_pcm_hw_params_set_buffer_size_near(device, hw_params,
		&frames)) < 0)
	{
		fprintf(stderr, "Error setting buffer_size %lu frames: %s\n", frames,
			snd_strerror(err));
		return 1;
	}
	
	if(buffer_size != frames * frame_size / fragments)
	{
		fprintf(stderr, "Could not set requested buffer size, %d != %lu\n",
			buffer_size, frames * frame_size / fragments);
		buffer_size = frames * frame_size / fragments;
	}

	if((err = snd_pcm_hw_params(device, hw_params)) < 0)
	{
		fprintf(stderr, "Error setting HW params: %s\n",
			snd_strerror(err));
		return 1;
	}
	
	return 0;
}

/*
 * catch ^C
 */
void handle_signals(int s)
{
	printf("Caught signal %d\n",s);
	exit_program = 1;
}

/*
 * audio thread
 */
void *audio_thread_handler(void *ptr)
{
	/* processing loop */
	fprintf(stderr, "Starting Audio Thread\n");
	while(!exit_program)
	{
		/* get input & handle errors */
		while((long)(inframes = snd_pcm_readi(capture_handle, rdbuf, frames)) < 0)
		{
			if(inframes == -EAGAIN)
				continue;
			
			if((err = snd_pcm_recover(capture_handle, (int)inframes, 1)))
				fprintf(stderr, "Input recover failed: %s\n",
					snd_strerror(err));
		}
		
		if(inframes != frames)
			fprintf(stderr, "Short read from capture device: %lu != %lu\n",
				inframes, frames);

		/* now processes the frames */
		Audio_Process(rdbuf, inframes);

		while((long)(outframes = snd_pcm_writei(playback_handle, rdbuf, inframes)) < 0)
		{
			if (outframes == -EAGAIN)
				continue;
			
			if((err = snd_pcm_recover(playback_handle, (int)outframes, 1)))
				fprintf(stderr, "Output recover failed: %s\n",
					snd_strerror(err));
		}
		
		if (outframes != inframes)
			fprintf(stderr, "Short write to playback device: %lu != %lu\n",
				outframes, frames);
	}
	
	fprintf(stderr, "Audio Thread Quitting.\n");
	return NULL;
}

/*
 * initialize the mixer
 */
void mixer_init(void)
{
	const char *card = "default";
	const char *selem_name = "Master";

	snd_mixer_open(&mixer_handle, 0);
	snd_mixer_attach(mixer_handle, card);
	snd_mixer_selem_register(mixer_handle, NULL, NULL);
	snd_mixer_load(mixer_handle);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name);
	elem = snd_mixer_find_selem(mixer_handle, sid);
}

/*
 * set mixer main level
 */
void mixer_set(long volume)
{
	long min, max;

	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);	
}

/*
 * top level
 */
int main(int argc, char **argv)
{
	pthread_t audio_thread;
	extern char *optarg;
	int opt;
	struct sigaction sigIntHandler;
	int i, verbose = 0, proc = 0;
	float amp = 0.6F, freq = 1000.0F;
	int iret;
    uint64_t samples;
    
	/* parse options */
	while((opt = getopt(argc, argv, "a:b:i:o:p:r:t:vVh")) != EOF)
	{
		switch(opt)
		{
			case 'a':
				/* amplitude */
				amp = atof(optarg);
				break;
			
			case 'b':
				/* buffer size */
				buffer_size = atoi(optarg);
				break;

			case 'i':
				/* input device */
				snd_device_in = optarg;
				break;

			case 'o':
				/* output device */
				snd_device_out = optarg;
				break;

			case 'p':
				/* process type */
				proc = atoi(optarg);
				break;

			case 'r':
				/* sample rate */
				sample_rate = atoi(optarg);
			
				/* search for rate */
				for(i=0;i<NUM_RATES;i++)
					if(sample_rate==legal_rates[i])
						break;
				if(i==3)
				{
					fprintf(stderr, "Illegal sample rate: %s\n", optarg);
					exit(1);
				}
				break;
            
            case 't':
                /* time delay */
				dlytime = atoi(optarg);
                break;
				
			case 'v':
				verbose = 1;
				break;
			
			case 'V':
				fprintf(stderr, "%s version %s\n", argv[0], swVersionStr);
				exit(0);
			
			case 'h':
			case '?':
				fprintf(stderr, "USAGE: %s [options]\n", argv[0]);
				fprintf(stderr, "Version %s, %s %s\n", swVersionStr, bdate, btime);
				fprintf(stderr, "Options: -a <amplitude  >    Default: %f\n", amp);
				fprintf(stderr, "         -b <Buffer Size>    Default: %d\n", buffer_size);
				fprintf(stderr, "         -f <freq>           Default: %f\n", freq);
				fprintf(stderr, "         -i <input device>   Default: %s\n", snd_device_in);
				fprintf(stderr, "         -o <output device>  Default: %s\n", snd_device_out);
				fprintf(stderr, "         -p <process type  > Default: %d\n", proc);
				fprintf(stderr, "         -r <sample rate Hz> Default: %d\n", sample_rate);
				fprintf(stderr, "         -t <time secs>      Default: %d\n", dlytime);
				fprintf(stderr, "         -v enables verbose progress messages\n");
				fprintf(stderr, "         -V prints the tool version\n");
				fprintf(stderr, "         -h prints this help\n");
				exit(1);
		}
	}
	
	/* set up audio processing */
    samples  = sample_rate * dlytime;
    if(samples > ((uint64_t)1<<32))
    {
		fprintf(stderr, "Requested time exceeds max\n");
        exit(1);
    }
	if(Audio_Init(samples, proc, amp, freq))
    {
		fprintf(stderr, "Audio Init failed\n");
        exit(1);
    }
	else
	{
		if(verbose)
			fprintf(stderr, "%lld sample buffer allocated\n", samples);
	}
    
	/* set up for control c */
	sigIntHandler.sa_handler = handle_signals;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);

	/* open audio devices - output first because input is slaved */
	if((err = snd_pcm_open(&playback_handle, snd_device_out, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
	{
		fprintf(stderr, "cannot open input audio device %s: %s\n", snd_device_in, snd_strerror(err));
		snd_pcm_close(capture_handle);
		exit(1);
	}
	
	if((err = snd_pcm_open(&capture_handle, snd_device_in, SND_PCM_STREAM_CAPTURE, 0)) < 0)
	{
		fprintf(stderr, "cannot open input audio device %s: %s\n", snd_device_out, snd_strerror(err));
		exit(1);
	}

	/* set up both devices identically */
	configure_alsa_audio(capture_handle,  nchannels);
	configure_alsa_audio(playback_handle, nchannels);
	
	/* init the mixer */
	//mixer_init();
	//mixer_set(play_vol);

	/* set up sizes */
	frame_size = nchannels * (bits / 8);
	fprintf(stderr, "Bytes/Frame = %d\n", frame_size);
	frames = buffer_size / frame_size;
	fprintf(stderr, "Frames/buffer = %lu\n", frames);
	
	/* allocate the audio buffer */
	rdbuf = (char *)malloc(buffer_size);
		
	/* prepare for use */
	snd_pcm_prepare(capture_handle);
	snd_pcm_prepare(playback_handle);

	/* fill the whole output buffer */
	for(i = 0; i < fragments; i += 1)
		snd_pcm_writei(playback_handle, rdbuf, frames);
	
	/* start audio thread */
	fprintf(stderr, "main: starting audio thread...\n");
	iret = pthread_create(&audio_thread, NULL, audio_thread_handler, NULL);
	if(!iret)
	{	
		/* wait for ^C */
		while(!exit_program)
		{
			/* wait a bit */
			usleep(100000);
			
            /* print some status */
            if(verbose)
                Audio_Status();
            
		}
		fprintf(stderr, "main: finishing...\n");
		
		pthread_join(audio_thread, NULL);
		fprintf(stderr, "main: audio thread joined...\n");
	}
	else
		fprintf(stderr, "main: error creating audio thread\n");
	
	/* clean up */
	snd_pcm_drain(playback_handle);
	snd_pcm_drop(capture_handle);
	free(rdbuf);
	//snd_mixer_close(mixer_handle);
	snd_pcm_close(playback_handle);
	snd_pcm_close(capture_handle);
    Audio_Close();
    
	return 0;
}
#else

#include <stdio.h>

int main() {
	printf("audio_fulldup: Hello, World!\n");

	return 0;
}

#endif