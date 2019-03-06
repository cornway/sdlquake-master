#include "main.h"
#include "quakedef.h"
#include "sound.h"
#include "SDL_audio.h"
#include "SDL_video.h"
#include "begin_code.h"
#include "SDL_keysym.h"

sizebuf_t   net_message;
int         demo_message_start;
int         net_activeconnections = 0;
int         vcrFile = -1;
cvar_t      hostname = {"hostname", "UNNAMED"};
qboolean    ipxAvailable = false;
char        my_ipx_address[NET_NAMELEN];
char        my_tcpip_address[NET_NAMELEN];
double      net_time;
qboolean    tcpipAvailable = false;
qboolean	serialAvailable = false;
int         net_hostport;
int         DEFAULTnet_hostport = 26000;
int         hostCacheCount = 0;
hostcache_t hostcache[HOSTCACHESIZE];

qboolean    slistInProgress = false;
qboolean    slistSilent = false;
qboolean    slistLocal = true;

cvar_t bgmvolume = {"bgmvolume", "1", true};
cvar_t volume = {"volume", "0.7", true};

int         desired_speed = 48000; //11025;
int         desired_bits = 16;

volatile dma_t  *shm = 0;

int recording = 0;

void (*GetComPortConfig) (int portNumber, int *port, int *irq, int *baud, qboolean *useModem) = NULL;
void (*SetComPortConfig) (int portNumber, int port, int irq, int baud, qboolean useModem) = NULL;
void (*GetModemConfig) (int portNumber, char *dialType, char *clear, char *init, char *hangup) = NULL;
void (*SetModemConfig) (int portNumber, char *dialType, char *clear, char *init, char *hangup) = NULL;

void NET_Slist_f (void)
{

}

int NET_SendUnreliableMessage (qsocket_t *sock, sizebuf_t *data)
{
    return -1;
}

qboolean NET_CanSendMessage (qsocket_t *sock)
{
    return false;
}

void NET_Close (qsocket_t *sock)
{
}

qsocket_t *NET_Connect (char *host)
{
    return NULL;
}

int NET_SendMessage (qsocket_t *sock, sizebuf_t *data)
{
    return -1;
}

int	NET_GetMessage (qsocket_t *sock)
{
    return -1;
}

void NET_Init (void)
{
}

void NET_Poll(void)
{
}

int NET_SendToAll(sizebuf_t *data, int blocktime)
{
    return -1;
}

void NET_Shutdown (void)
{
}

qsocket_t *NET_CheckNewConnections (void)
{
    return NULL;
}

void S_StopAllSounds(qboolean clear)
{
}

void S_StopSound(int entnum, int entchannel)
{
}

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



void S_ClearPrecache (void)
{
}

void S_BeginPrecaching (void)
{
}

void S_EndPrecaching (void)
{
}


sfx_t *S_PrecacheSound (char *name)
{
    return NULL;
}


void S_StartSound(int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol, float attenuation)
{
}

void S_PaintChannels(int endtime)
{

}

void S_StaticSound (sfx_t *sfx, vec3_t origin, float vol, float attenuation)
{
}

void S_TouchSound (char *name)
{
}

void S_LocalSound (char *sound)
{
}

void S_ExtraUpdate (void)
{
}

void S_Init (void)
{
}

void S_Shutdown(void)
{
}

void S_Update(vec3_t origin, vec3_t forward, vec3_t right, vec3_t up)
{

}

void S_ClearBuffer (void)
{

}

DECLSPEC int SDLCALL SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained)
{
    return 0;
}

DECLSPEC void SDLCALL SDL_CloseAudio(void)
{
    
}

DECLSPEC void SDLCALL SDL_PauseAudio(int pause_on)
{

}

DECLSPEC char * SDLCALL SDL_GetError(void)
{
    return "not implemented yet\n";
}

DECLSPEC void SDLCALL SDL_ClearError(void)
{

}

DECLSPEC SDLMod SDLCALL SDL_GetModState(void)
{
    return KMOD_NONE;
}

DECLSPEC Uint8 SDLCALL SDL_GetMouseState(int *x, int *y)
{
    return 0;
}

DECLSPEC void SDLCALL SDL_Quit(void)
{
    fatal_error("quit/n");
}

DECLSPEC void SDLCALL SDL_UpdateRect
        (SDL_Surface *screen, Sint32 x, Sint32 y, Uint32 w, Uint32 h)
{

}


