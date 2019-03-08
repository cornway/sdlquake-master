
#include <stdio.h>
#include "SDL_audio.h"
#include "SDL_byteorder.h"
#include "quakedef.h"
#include "audio_main.h"
#include "sound.h"
#include "begin_code.h"

#if 0
static dma_t the_shm;
static int snd_inited;

extern int desired_speed;
extern int desired_bits;

static void paint_audio(void *unused, Uint8 *stream, int len)
{
	if ( shm ) {
		shm->buffer = stream;
		shm->samplepos += len/(shm->samplebits/8)/2;
		// Check for samplepos overflow?
		S_PaintChannels (shm->samplepos);
	}
}

qboolean SNDDMA_Init(void)
{
	SDL_AudioSpec desired, obtained;

	snd_inited = 0;

	/* Set up the desired format */
	desired.freq = desired_speed;
	switch (desired_bits) {
		case 8:
			desired.format = AUDIO_U8;
			break;
		case 16:
			if ( SDL_BYTEORDER == SDL_BIG_ENDIAN )
				desired.format = AUDIO_S16MSB;
			else
				desired.format = AUDIO_S16LSB;
			break;
		default:
        		Con_Printf("Unknown number of audio bits: %d\n",
								desired_bits);
			return 0;
	}
	desired.channels = 2;
	desired.samples = 512;
	desired.callback = paint_audio;

	/* Open the audio device */
	if ( SDL_OpenAudio(&desired, &obtained) < 0 ) {
        	Con_Printf("Couldn't open SDL audio: %s\n", SDL_GetError());
		return 0;
	}

	/* Make sure we can support the audio format */
	switch (obtained.format) {
		case AUDIO_U8:
			/* Supported */
			break;
		case AUDIO_S16LSB:
		case AUDIO_S16MSB:
			if ( ((obtained.format == AUDIO_S16LSB) &&
			     (SDL_BYTEORDER == SDL_LIL_ENDIAN)) ||
			     ((obtained.format == AUDIO_S16MSB) &&
			     (SDL_BYTEORDER == SDL_BIG_ENDIAN)) ) {
				/* Supported */
				break;
			}
			/* Unsupported, fall through */;
		default:
			/* Not supported -- force SDL to do our bidding */
			SDL_CloseAudio();
			if ( SDL_OpenAudio(&desired, NULL) < 0 ) {
        			Con_Printf("Couldn't open SDL audio: %s\n",
							SDL_GetError());
				return 0;
			}
			memcpy(&obtained, &desired, sizeof(desired));
			break;
	}
	SDL_PauseAudio(0);

	/* Fill the audio DMA information block */
	shm = &the_shm;
	shm->splitbuffer = 0;
	shm->samplebits = (obtained.format & 0xFF);
	shm->speed = obtained.freq;
	shm->channels = obtained.channels;
	shm->samples = obtained.samples*shm->channels;
	shm->samplepos = 0;
	shm->submission_chunk = 1;
	shm->buffer = NULL;

	snd_inited = 1;
	return 1;
}

int SNDDMA_GetDMAPos(void)
{
	return shm->samplepos;
}

void SNDDMA_Shutdown(void)
{
	if (snd_inited)
	{
		SDL_CloseAudio();
		snd_inited = 0;
	}
}

#else

int sound_started=0;

cvar_t bgmvolume = {"bgmvolume", "1", true};
cvar_t volume = {"volume", "0.7", true};

int         desired_speed = 11025; //11025;
int         desired_bits = 16;

volatile dma_t  *shm = 0;

int recording = 0;

cvar_t nosound = {"nosound", "0"};
cvar_t precache = {"precache", "1"};

channel_t   channels[MAX_CHANNELS];
int			total_channels;

vec3_t		listener_origin;
vec3_t		listener_forward;
vec3_t		listener_right;
vec3_t		listener_up;
vec_t		sound_nominal_clip_dist=1000.0;

int			soundtime;		// sample PAIRS
int   		paintedtime; 	// sample PAIRS


void CDAudio_Pause(void)
{
}

void CDAudio_Play(byte track, qboolean looping)
{
}


void CDAudio_Resume(void)
{
}

int CDAudio_Init(void)
{
    return -1;
}

void CDAudio_Shutdown(void)
{
}

void CDAudio_Update(void)
{
}


void S_PaintChannels(int endtime)
{}


void S_StopAllSounds(qboolean clear)
{
}

void S_ClearPrecache (void)
{}


void S_BeginPrecaching (void)
{
}

void S_EndPrecaching (void)
{
}


/*
=================
SND_PickChannel
=================
*/
channel_t *SND_PickChannel(int entnum, int entchannel, int *chidx)
{
    int ch_idx;
    int first_to_die;
    int life_left;

// Check for replacement sound, or find the best one to replace
    first_to_die = -1;
    life_left = 0x7fffffff;
    for (ch_idx=NUM_AMBIENTS ; ch_idx < NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS ; ch_idx++)
    {

    if (entchannel != 0		// channel 0 never overrides
    && channels[ch_idx].entnum == entnum
    && (channels[ch_idx].entchannel == entchannel || entchannel == -1) )
    {   // allways override sound from same entity
        first_to_die = ch_idx;
        break;
    }

    // don't let monster sounds override player sounds
    if (channels[ch_idx].entnum == cl.viewentity && entnum != cl.viewentity && channels[ch_idx].sfx)
        continue;

        if (channels[ch_idx].end - paintedtime < life_left)
        {
            life_left = channels[ch_idx].end - paintedtime;
            first_to_die = ch_idx;
        }
    }

    if (first_to_die == -1)
        return NULL;

    if (channels[first_to_die].sfx)
        channels[first_to_die].sfx = NULL;

    *chidx = first_to_die;
    return &channels[first_to_die];
}       


extern sfxcache_t *S_LoadSound (sfx_t *s);

sfx_t *S_FindName (char *name, sfx_t *sfx)
{
    strcpy (sfx->name, name);
    return sfx;
}

/*
==================
S_PrecacheSound

==================
*/
sfx_t *S_PrecacheSound (char *name, sfx_t *sfx)
{
    if (!sound_started || nosound.value)
        return NULL;


    S_FindName (name, sfx);

// cache it in
    if (precache.value)
        S_LoadSound (sfx);

    return sfx;
}

/*
==================
S_TouchSound

==================
*/
void S_TouchSound (char *name, sfx_t	*sfx)
{
    if (!sound_started)
        return;

    sfx = S_FindName (name, sfx);
    Cache_Check (&sfx->cache);
}


void S_LocalSound (char *sound)
{
}

void S_ExtraUpdate (void)
{
}

/*
=================
SND_Spatialize
=================
*/
void SND_Spatialize(channel_t *ch)
{
    vec_t dot;
    vec_t ldist, rdist, dist;
    vec_t lscale, rscale, scale;
    vec3_t source_vec;
    sfx_t *snd;

// anything coming from the view entity will allways be full volume
    if (ch->entnum == cl.viewentity)
    {
        ch->leftvol = ch->master_vol;
        ch->rightvol = ch->master_vol;
        return;
    }

// calculate stereo seperation and distance attenuation

    snd = ch->sfx;
    VectorSubtract(ch->origin, listener_origin, source_vec);

    dist = VectorNormalize(source_vec) * ch->dist_mult;

    dot = DotProduct(listener_right, source_vec);

    if (shm->channels == 1)
    {
        rscale = 1.0;
        lscale = 1.0;
    }
    else
    {
        rscale = 1.0 + dot;
        lscale = 1.0 - dot;
    }

// add in distance effect
    scale = (1.0 - dist) * rscale;
    ch->rightvol = (int) (ch->master_vol * scale);
    if (ch->rightvol < 0)
        ch->rightvol = 0;

    scale = (1.0 - dist) * lscale;
    ch->leftvol = (int) (ch->master_vol * scale);
    if (ch->leftvol < 0)
        ch->leftvol = 0;
}


static void S_PushSound (channel_t *ch, int channel)
{
    Mix_Chunk chunk;
    sfxcache_t	*sc;
    audio_channel_t *ach;

    sc = Cache_Check (&ch->sfx->cache);

    chunk.abuf = (snd_sample_t *)sc->data;
    chunk.alen = sc->length * sizeof(snd_sample_t);
    chunk.volume = (ch->leftvol + ch->rightvol) / 4;
    if (audio_is_playing(channel)) {
        audio_stop_channel(channel);
    }///*
    ach = audio_play_channel(&chunk, channel);
    if (!ach) {
        Sys_Error("");
    }
    ach->complete = NULL;
    //*/
}

void S_StartSound(int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol, float attenuation)
{
    channel_t *target_chan, *check;
    sfxcache_t	*sc;
    int		vol;
    int		ch_idx, chpush;
    int		skip;

    if (!sound_started)
        return;

    if (!sfx)
        return;

    if (nosound.value)
        return;

    vol = fvol*255;

// pick a channel to play on
    target_chan = SND_PickChannel(entnum, entchannel, &chpush);
    if (!target_chan)
        return;

// spatialize
    memset (target_chan, 0, sizeof(*target_chan));
    VectorCopy(origin, target_chan->origin);
    target_chan->dist_mult = attenuation / sound_nominal_clip_dist;
    target_chan->master_vol = vol;
    target_chan->entnum = entnum;
    target_chan->entchannel = entchannel;
    SND_Spatialize(target_chan);

    if (!target_chan->leftvol && !target_chan->rightvol)
        return;		// not audible at all

// new channel
    sc = S_LoadSound (sfx);
    if (!sc)
    {
        target_chan->sfx = NULL;
        return;// couldn't load the sound's data
    }

    target_chan->sfx = sfx;
    target_chan->pos = 0.0;
    target_chan->end = paintedtime + sc->length;	

// if an identical sound has also been started this frame, offset the pos
// a bit to keep it from just making the first one louder
    check = &channels[NUM_AMBIENTS];
    for (ch_idx=NUM_AMBIENTS ; ch_idx < NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS ; ch_idx++, check++)
    {
        if (check == target_chan)
            continue;
        if (check->sfx == sfx && !check->pos)
        {
            skip = rand () % (int)(0.1*shm->speed);
            if (skip >= target_chan->end)
                skip = target_chan->end - 1;
            target_chan->pos += skip;
            target_chan->end -= skip;
            break;
        }
    }
    S_PushSound(target_chan, chpush);
}

void S_StopSound(int entnum, int entchannel)
{
    int i;

    for (i=0 ; i<MAX_DYNAMIC_CHANNELS ; i++)
    {
        if (channels[i].entnum == entnum
            && channels[i].entchannel == entchannel)
        {
            channels[i].end = 0;
            channels[i].sfx = NULL;
            audio_stop_channel(i);
            return;
        }
    }
}

void S_StaticSound (sfx_t *sfx, vec3_t origin, float vol, float attenuation)
{}


void S_Init (void)
{
    Con_Printf("\nSound Initialization\n");

    if (COM_CheckParm("-nosound"))
        return;



    Cvar_RegisterVariable(&nosound);
    Cvar_RegisterVariable(&volume);
    Cvar_RegisterVariable(&precache);
    Cvar_RegisterVariable(&bgmvolume);

    if (host_parms.memsize < 0x800000)
    {
        Cvar_Set ("loadas8bit", "1");
        Con_Printf ("loading all sounds as 8bit\n");
    }



    sound_started = true;

    shm = (void *) Hunk_AllocName(sizeof(*shm), "shm");
    shm->splitbuffer = 0;
    shm->samplebits = 16;
    shm->speed = AUDIO_SAMPLE_RATE;
    shm->channels = 2;
    shm->samples = AUDIO_OUT_BUFFER_SIZE;
    shm->samplepos = 0;
    shm->soundalive = true;
    shm->gamealive = true;
    shm->submission_chunk = 1;

    S_StopAllSounds (true);
}

void S_Shutdown(void)
{
}

void S_Update(vec3_t origin, vec3_t forward, vec3_t right, vec3_t up)
{
    int         i, j;
    int         total;
    channel_t   *ch;
    channel_t   *combine;

    if (!sound_started)
        return;

    VectorCopy(origin, listener_origin);
    VectorCopy(forward, listener_forward);
    VectorCopy(right, listener_right);
    VectorCopy(up, listener_up);
    
// update general area ambient sound sources
    //S_UpdateAmbientSounds ();

    combine = NULL;

// update spatialization for static and dynamic sounds  
    ch = channels+NUM_AMBIENTS;
    for (i=NUM_AMBIENTS ; i<total_channels; i++, ch++)
    {
        if (!ch->sfx)
            continue;
        SND_Spatialize(ch);         // respatialize channel
        if (!ch->leftvol && !ch->rightvol)
            continue;

    // try to combine static sounds with a previous channel of the same
    // sound effect so we don't mix five torches every frame
    
        if (i >= MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS)
        {
        // see if it can just use the last one
            if (combine && combine->sfx == ch->sfx)
            {
                combine->leftvol += ch->leftvol;
                combine->rightvol += ch->rightvol;
                ch->leftvol = ch->rightvol = 0;
                continue;
            }
        // search for one
            combine = channels+MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS;
            for (j=MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS ; j<i; j++, combine++)
                if (combine->sfx == ch->sfx)
                    break;
                    
            if (j == total_channels)
            {
                combine = NULL;
            }
            else
            {
                if (combine != ch)
                {
                    combine->leftvol += ch->leftvol;
                    combine->rightvol += ch->rightvol;
                    ch->leftvol = ch->rightvol = 0;
                }
                continue;
            }
        }
        
        
    }

// mix some sound
    //S_Update_();

    audio_update();
}

void S_ClearBuffer (void)
{

}

DECLSPEC int SDLCALL SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained)
{
    return 0;
}

DECLSPEC void SDLCALL SDL_CloseAudio(void)
{}


#endif

