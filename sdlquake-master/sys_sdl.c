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
#include "ff.h"
#include "debug.h"
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

#define MAX_HANDLES		3

typedef struct {
    FIL file;
    int is_owned;
} fhandle_t;

fhandle_t sys_handles[MAX_HANDLES];

static FIL *allochandle (int *num)
{
    int i;

    for (i=0 ; i<MAX_HANDLES ; i++) {
        if (sys_handles[i].is_owned == 0) {
            sys_handles[i].is_owned = 1;
            *num = i;
            return &sys_handles[i].file;
        }
    }
    return NULL;
}

static inline FIL *gethandle (int num)
{
    return &sys_handles[num].file;
}

static void releasehandle (int handle)
{
    sys_handles[handle].is_owned = 0;
}

int Sys_FileOpenRead (char *path, int *hndl)
{
    FRESULT res;
    FIL *f;

    f = allochandle(hndl);
    if (f == NULL) {
        *hndl = -1;
        return -1;
    }
    res = f_open(f, path, FA_OPEN_EXISTING | FA_READ);
    if (res != FR_OK) {
        releasehandle(*hndl);
        *hndl = -1;
        return -1;
    }

    return f_size(f);
}

int Sys_FileOpenWrite (char *path)
{
    FRESULT res;
    FIL *f;
    int i;

    Sys_Error("Not supported yet");

    f = allochandle(&i);
    if (f == NULL) {
        return -1;
    }

    res = f_open(f, path, FA_OPEN_EXISTING | FA_WRITE);
    if (res != FR_OK) {
        releasehandle(i);
        return -1;
    }

    return i;
}

void Sys_FileClose (int handle)
{
    if ( handle >= 0 ) {
        f_close(gethandle(handle));
        releasehandle(handle);
    }
}

void Sys_FileSeek (int handle, int position)
{
    if ( handle >= 0 ) {
        f_lseek(gethandle(handle), position);
    }
}

int Sys_Feof (int handle)
{
    if (handle < 0) {
        return handle;
    }
    return f_eof(gethandle(handle));
}

int Sys_FileRead (int handle, void *dst, int count)
{
    char *data;
    UINT done = 0;
    FRESULT res = FR_NOT_READY;

    if ( handle >= 0 ) {
        data = dst;
        res = f_read(gethandle(handle), data, count, &done);
    }
    if (res != FR_OK) {
        Sys_Error("Could not read file from handle : %d\n", handle);
    }
    return done;
}

char *Sys_FileGetS (int handle, char *dst, int count)
{
    if (f_gets(dst, count, gethandle(handle)) == NULL) {
        Sys_Error("Could not read file from handle : %d\n", handle);
    }
    return dst;
}

int Sys_FileWrite (int handle, void *src, int count)
{
    char *data;
    UINT done;
    FRESULT res = FR_NOT_READY;

    if ( handle >= 0 ) {
        data = src;
        res = f_write (gethandle(handle), data, count, &done);
    }
    if (res != FR_OK) {
        Sys_Error("Could not write file from handle : %d\n", handle);
    }
    return done;
}

int Sys_FPrintf (int handle, char *fmt, ...)
{
    FRESULT res = FR_NOT_READY;
    va_list ap;
    char p[256];
    int   r;

    va_start (ap, fmt);
    r = vsnprintf(p, sizeof(p), fmt, ap);
    va_end (ap);

    if (handle < 0) {
        return handle;
    }
    res = f_printf(gethandle(handle), fmt);
    if (res != FR_OK) {
        return -1;
    }
    return FR_OK;
}


int	Sys_FileTime (char *path)
{
    return 0;
}

void Sys_mkdir (char *path)
{
#ifdef __WIN32__
    mkdir (path);
#else
    FRESULT res;
    static DIR dp;
    res = f_opendir(&dp, path);
    if (res != FR_OK) {
        Sys_Error("Mkdir fail for path : %s\n", path);
    }
#endif
}

void Sys_DebugLog(char *file, char *fmt, ...)
{

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


/*
================
Sys_MakeCodeWriteable
================
*/
void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
#if 0
	int r;
	unsigned long addr;
	int psize = getpagesize();

	fprintf(stderr, "writable code %lx-%lx\n", startaddr, startaddr+length);

	addr = startaddr & ~(psize-1);

	r = mprotect((char*)addr, length + startaddr - addr, 7);

	if (r < 0)
    		Sys_Error("Protection change failed\n");
#endif
}

