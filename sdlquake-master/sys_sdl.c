/* -*- Mode: C; tab-width: 4 -*- */ 

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#ifndef __WIN32__

#endif

#include "quakedef.h"
#include "sdl_keysym.h"
#include "audio_main.h"
#include "input_main.h"
#include "main.h"
#include "dev_io.h"
#include "debug.h"
#include <misc_utils.h>
#include "begin_code.h"

qboolean        isDedicated;

int noconinput = 0;

char *basedir = ".";
char *cachedir = "/tmp";

Q_CVAR_DEF(sys_linerefresh, "sys_linerefresh", 0);// set for entity display
Q_CVAR_DEF(sys_nostdout, "sys_nostdout", 0);

// =======================================================================
// General routines
// =======================================================================

void Sys_DebugNumber(int y, int val)
{
}

void Sys_Printf (char *fmt, ...)
{
    va_list         argptr;

    va_start (argptr, fmt);
    dvprintf (fmt, argptr);
    va_end (argptr);
}

void Sys_Quit (void)
{
	Host_Shutdown();
	for (;;) {}
}

void Sys_Init(void)
{
#if id386
	Sys_SetFPCW();
#endif
}

void SDL_Quit(void)
{
    Sys_Error("-----------SDL_Quit-----------/n");
}

#if !id386

/*
================
Sys_LowFPPrecision
================
*/
void Sys_LowFPPrecision (void)
{
// causes weird problems on Nextstep
}


/*
================
Sys_HighFPPrecision
================
*/
void Sys_HighFPPrecision (void)
{
// causes weird problems on Nextstep
}

#endif	// !id386


void Sys_Error (char *error, ...)
{
    va_list         argptr;

    va_start (argptr, error);
    dvprintf (error, argptr);
    va_end (argptr);

    serial_flush();
    Sys_Quit();
} 

void Sys_Warn (char *warning, ...)
{ 
    va_list         argptr;

    va_start (argptr, warning);
    dvprintf (warning, argptr);
    va_end (argptr);
}

void Sys_DebugLog(char *file, char *fmt, ...)
{
    va_list         argptr;
    
    va_start (argptr, fmt);
    dvprintf (fmt, argptr);
    va_end (argptr);
}

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


/*
===============================================================================

FILE IO

===============================================================================
*/
int Sys_FileOpenRead (char *path, int *hndl)
{
    return d_open(path, hndl, "r");
}

int Sys_FileOpenWrite (char *path)
{
    int h;
    d_open(path, &h, "+w");
    return h;
}

void Sys_FileClose (int handle)
{
    d_close(handle);
}

void Sys_FileSeek (int handle, int position)
{
    d_seek(handle, position, DSEEK_SET);
}

int Sys_Feof (int handle)
{
    return d_eof(handle);
}

int Sys_FileRead (int handle, void *dst, int count)
{
    return d_read(handle, dst, count);
}

char *Sys_FileGetS (int handle, char *dst, int count)
{
    return d_gets(handle, dst, count);
}

char Sys_FileGetC (int handle)
{
    return d_getc(handle);
}

int Sys_FileWrite (int handle, void *src, int count)
{
    return d_write(handle, src, count);
}

int Sys_FPrintf (int handle, char *fmt, ...)
{
    va_list ap;
    char p[256];
    int   r;

    va_start (ap, fmt);
    r = vsnprintf(p, sizeof(p), fmt, ap);
    va_end (ap);
    if (Sys_FileWrite(handle, p, r) < 0) {
        dprintf("%s Bad : %s\n", __func__, p);
    }
    return r;
}


int	Sys_FileTime (char *path)
{
    return d_time();
}

void Sys_mkdir (char *path)
{
    d_mkdir(path);
}


extern volatile uint32_t systime;

double Sys_FloatTime (void)
{
#ifdef __WIN32__

	static int starttime = 0;

	if ( ! starttime )
		starttime = clock();

	return (clock()-starttime)*1.0/1024;

#else

    return systime;

#endif
}

// =======================================================================
// Sleeps for microseconds
// =======================================================================

static volatile int oktogo;

void alarm_handler(int x)
{
	oktogo=1;
}

byte *Sys_ZoneBase (int *size)
{

    Sys_Error("Not supported");
    return NULL;
}

void Sys_LineRefresh(void)
{
}

void Sys_Sleep(void)
{
	HAL_Delay(1);
}

void floating_point_exception_handler(int whatever)
{
    Sys_Error("floating point exception\n");
}

void moncontrol(int x)
{
}

int SDL_main (int argc, const char *argv[])
{

    double  time, oldtime, newtime;
    quakeparms_t parms;
    extern int vcrFile;
    extern int recording;
    static int frame;

    moncontrol(0);

    parms.memsize = Sys_AllocBytesLeft();
    parms.membase = Sys_AllocShared(&parms.memsize);
    parms.basedir = basedir;
    // Disable cache, else it looks in the cache for config.cfg.
    parms.cachedir = NULL;

    COM_InitArgv(argc, (char **)argv);
    parms.argc = com_argc;
    parms.argv = com_argv;

    Sys_Init();

    Host_Init(&parms);

    Cvar_RegisterVariable (&sys_nostdout);

    oldtime = Sys_FloatTime () - 0.1;
    while (1)
    {
// find time spent rendering last frame
        newtime = Sys_FloatTime ();
        time = newtime - oldtime;

        if (cls.state == ca_dedicated)
        {   // play vcrfiles at max speed
            if (time < sys_ticrate.value && (vcrFile == -1 || recording) )
            {
                HAL_Delay (1);
                continue;       // not time to run a server only tic yet
            }
            time = sys_ticrate.value;
        }

        if (time > sys_ticrate.value*2)
            oldtime = newtime;
        else
            oldtime += time;

        if (++frame > 10)
            moncontrol(1);      // profile only while we do each Quake frame
        Host_Frame (time);
        moncontrol(0);

// graphic debugging aids
        if (sys_linerefresh.value)
            Sys_LineRefresh ();

        input_tickle();
    }

}

static uint8_t *syscache = NULL;
static uint8_t *syscache_top = NULL,
            *syscache_bot = NULL;

static uint32_t syscahce_size = (1024 * 700); /*~700 Kb*/

void Sys_CacheInit (void)
{
    syscache = Sys_Malloc(syscahce_size);
    assert(syscache);
    syscache_top = syscache + syscahce_size;
    syscache_bot = syscache;
}

void *Sys_HeapCacheTop (int size) /*alloc forever*/
{
    if (syscahce_size < size) {
        return NULL;
    }
    syscahce_size = syscahce_size - size;
    syscache_top = syscache_top - size;
    return (void *)syscache_top;
}

void Sys_HeapCachePush (int size) /*dealloc*/
{
    syscache_bot = syscache_bot - size;
    syscahce_size = syscahce_size + size;
}

extern void *Sys_HeapCachePop (int size) /*alloc*/
{
    void *ptr;
    if (syscahce_size < size) {
        return NULL;
    }
    ptr = syscache_bot;
    syscache_bot = syscache_bot + size;
    syscahce_size = syscahce_size - size;
    return (void *)ptr;
}

#if	id386
/*
================
Sys_MakeCodeWriteable
================
*/
void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
	int r;
	unsigned long addr;
	int psize = getpagesize();

	fprintf(stderr, "writable code %lx-%lx\n", startaddr, startaddr+length);

	addr = startaddr & ~(psize-1);

	r = mprotect((char*)addr, length + startaddr - addr, 7);

	if (r < 0)
    		Sys_Error("Protection change failed\n");
}

#endif /*id386*/

