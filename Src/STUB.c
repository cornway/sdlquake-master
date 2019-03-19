#include "quakedef.h"
#include "SDL_keysym.h"
#include "console.h"
#include "begin_code.h"

qsocket_t localhost;

int         demo_message_start;

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
