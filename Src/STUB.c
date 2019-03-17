#include "quakedef.h"
#include "SDL_keysym.h"
#include "begin_code.h"

sizebuf_t   net_message;
int         demo_message_start;
int         net_activeconnections = 0;
int         vcrFile = -1;
#if CVAR_TINY
Q_CVAR_DEF(hostname, "hostname", 0);
#else
Q_CVAR_DEF(hostname, "hostname", "UNNAMED");
#endif
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

DECLSPEC void SDLCALL SDL_PauseAudio(int pause_on)
{}

DECLSPEC char * SDLCALL SDL_GetError(void)
{
    return "not implemented yet\n";
}

DECLSPEC void SDLCALL SDL_ClearError(void)
{}

DECLSPEC SDLMod SDLCALL SDL_GetModState(void)
{
    return KMOD_NONE;
}

DECLSPEC uint8_t SDLCALL SDL_GetMouseState(int *x, int *y)
{
    return 0;
}

void SDL_Quit(void)
{
    fatal_error("-----------SDL_Quit-----------/n");
}


