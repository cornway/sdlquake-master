// vid_sdl.h -- sdl video driver 

#include <config.h>

#include <lcd_main.h>
#include <misc_utils.h>
#include <input_main.h>
#include <heap.h>
#include <gfx.h>
#include <bsp_sys.h>

#include "quakedef.h"
#include "d_local.h"
#include "sdl_video.h"
#include "sdl_keysym.h"

#define VIDEO_IN_IRAM 1
#define pal_t uint32_t
#define pix_t uint8_t

viddef_t    vid;                // global video state
unsigned short  d_8to16table[256];

// The original defaults
//#define    BASEWIDTH    320
//#define    BASEHEIGHT   200
// Much better for high resolution displays

#if VIDEO_IN_IRAM
pix_t screenbuf[BASEWIDTH * BASEHEIGHT * sizeof(pix_t) + sizeof(SDL_Surface)] = {0};
#endif

int    VGA_width, VGA_height, VGA_rowbytes, VGA_bufferrowbytes = 0;
byte    *VGA_pagebase;

static SDL_Surface *screen = NULL;

static qboolean mouse_avail;
static float   mouse_x, mouse_y;
static int mouse_oldbuttonstate = 0;

// No support for option menus
void (*vid_menudrawfn)(void) = NULL;
void (*vid_menukeyfn)(int key) = NULL;

void VID_SetPalette (byte* palette)
{
    unsigned int i;
    pal_t pal[256];
    byte r, g, b;

    for (i = 0; i < 256; i++)
    {
        r = *palette++;
        g = *palette++;
        b = *palette++;
        pal[i] = GFX_RGBA8888(r, g, b, 0xff);
    }
    vid_set_clut(pal, 256);
    return;
}


void    VID_ShiftPalette (unsigned char *palette)
{
    VID_SetPalette(palette);
}

void VID_PreConfig (void)
{
    screen_conf_t conf;
    int hwaccel = 0, p;

    p = bsp_argv_check("-gfxmod");
    if (p >= 0) {
        const char *str = bsp_argv_get(p);
        hwaccel = atoi(str);
    }

    conf.res_x = BASEWIDTH;
    conf.res_y = BASEHEIGHT;
    conf.alloc.malloc = heap_alloc_shared;
    conf.alloc.free = heap_free;
    conf.colormode = GFX_COLOR_MODE_CLUT;
    conf.laynum = 2;
    conf.hwaccel = hwaccel;
    conf.clockpresc = 1;
    vid_config(&conf);
}

void    VID_Init (unsigned char *palette)
{
    int chunk;
    byte *cache;
    int cachesize;
    Uint32 flags;
    screen_t lcd_screen;

    // Set up display mode (width and height)
    vid.width = BASEWIDTH;
    vid.height = BASEHEIGHT;
    vid.maxwarpwidth = WARP_WIDTH;
    vid.maxwarpheight = WARP_HEIGHT;

    lcd_screen.buf = NULL;
    lcd_screen.width = BASEWIDTH;
    lcd_screen.height = BASEHEIGHT;

#if VIDEO_IN_IRAM
    screen = (SDL_Surface *)&screenbuf[0];
#else
    screen = (SDL_Surface *)Hunk_HighAllocName(BASEWIDTH * BASEHEIGHT * sizeof(pix_t) + sizeof(SDL_Surface), "screen");
    if (screen == NULL)
        Sys_Error ("Not enough memory for video mode\n");
#endif

    // Set video width, height and flags
    flags = (SDL_SWSURFACE|SDL_HWPALETTE|SDL_FULLSCREEN);

    memset(screen, 0, sizeof(SDL_Surface));

    screen->pixels = (void *)(screen + 1);
    screen->flags = flags;
    screen->w = BASEWIDTH;
    screen->h = BASEHEIGHT;
    screen->offset = 0;
    screen->pitch = BASEWIDTH;

    VID_SetPalette(palette);

    // now know everything we need to know about the buffer
    VGA_width = vid.conwidth = vid.width;
    VGA_height = vid.conheight = vid.height;
    vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
    vid.numpages = 1;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
    VGA_pagebase = vid.buffer = screen->pixels;
    VGA_rowbytes = vid.rowbytes = screen->pitch;
    vid.conbuffer = vid.buffer;
    vid.conrowbytes = vid.rowbytes;
    vid.direct = 0;

    // allocate z buffer and surface cache
    chunk = vid.width * vid.height * sizeof (*d_pzbuffer);
    cachesize = D_SurfaceCacheForRes (vid.width, vid.height);
    chunk += cachesize;
    d_pzbuffer = Hunk_HighAllocName(chunk, "video");
    if (d_pzbuffer == NULL)
        Sys_Error ("Not enough memory for video mode\n");

    // initialize the cache memory
    cache = (byte *) d_pzbuffer
                + vid.width * vid.height * sizeof (*d_pzbuffer);
    D_InitCaches (cache, cachesize);

    chunk += cachesize;
    d_pzbuffer = Hunk_HighAllocName(chunk, "video");
    if (d_pzbuffer == NULL)
        Sys_Error ("Not enough memory for video mode\n");

    // initialize the cache memory
    cache = (byte *) d_pzbuffer
                + vid.width * vid.height * sizeof (*d_pzbuffer);
    D_InitCaches (cache, cachesize);
}

void    VID_Shutdown (void)
{
    SDL_Quit();
}

void uiUpdate (vrect_t *rect, screen_t *lcd_screen)
{
    vid_update(lcd_screen);
}

void    VID_Update (vrect_t *rects)
{
    vrect_t *rect;
    screen_t lcd_screen = {0};
    lcd_screen.buf = VGA_pagebase;
    lcd_screen.width = BASEWIDTH;
    lcd_screen.height = BASEHEIGHT;

    for (rect = rects; rect; rect = rect->pnext) {
        uiUpdate(rect, &lcd_screen);
    }
}

/*
================
D_BeginDirectRect
================
*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
    uint8_t *offset;


    if (!screen) return;
    if ( x < 0 ) x = screen->w+x-1;
    offset = (uint8_t *)screen->pixels + y*screen->pitch + x;
    while ( height-- )
    {
        d_memcpy(offset, pbitmap, width);
        offset += screen->pitch;
        pbitmap += width;
    }
}


/*
================
D_EndDirectRect
================
*/
void D_EndDirectRect (int x, int y, int width, int height)
{
    if (!screen) return;
    if (x < 0) x = screen->w+x-1;
    //SDL_UpdateRect(screen, x, y, width, height);
}


/*
================
Sys_SendKeyEvents
================
*/

const kbdmap_t gamepad_to_kbd_map[JOY_STD_MAX] =
{
    [JOY_UPARROW]       = {K_UPARROW, 0},
    [JOY_DOWNARROW]     = {K_DOWNARROW, 0},
    [JOY_LEFTARROW]     = {K_LEFTARROW,0},
    [JOY_RIGHTARROW]    = {K_RIGHTARROW, 0},
    [JOY_K1]            = {'/', PAD_FREQ_LOW},
    [JOY_K4]            = {K_END,  0},
    [JOY_K3]            = {K_CTRL, 0},
    [JOY_K2]            = {K_SPACE,    0},
    [JOY_K5]            = {'a',    0},
    [JOY_K6]            = {'d',    0},
    [JOY_K7]            = {K_DEL,  0},
    [JOY_K8]            = {K_PGDN, 0},
    [JOY_K9]            = {K_ENTER, 0},
    [JOY_K10]           = {K_ESCAPE, PAD_FREQ_LOW},
};

static i_event_t *__post_key (i_event_t *events, i_event_t *event)
{
    Key_Event(event->sym, (qboolean)event->state);
    return NULL;
}

void Sys_SendKeyEvents(void)
{
    input_proc_keys(NULL);
}

void IN_Init (void)
{
    input_soft_init(__post_key, gamepad_to_kbd_map);
    input_bind_extra(K_EX_LOOKUP, K_HOME);
    input_bind_extra(K_EX_LOOKUP, K_DEL);
    input_bind_extra(K_EX_LOOKUP, K_INS);

    if ( COM_CheckParm ("-nomouse") )
        return;
    mouse_x = mouse_y = 0.0;
    mouse_avail = 1;
}

void IN_Shutdown (void)
{
    mouse_avail = 0;
}

void IN_Commands (void)
{
    int i;
    int mouse_buttonstate;
   
    if (!mouse_avail) return;
   
    i = SDL_GetMouseState(NULL, NULL);
    /* Quake swaps the second and third buttons */
    mouse_buttonstate = (i & ~0x06) | ((i & 0x02)<<1) | ((i & 0x04)>>1);
    for (i=0 ; i<3 ; i++) {
        if ( (mouse_buttonstate & (1<<i)) && !(mouse_oldbuttonstate & (1<<i)) )
            Key_Event (K_MOUSE1 + i, true);

        if ( !(mouse_buttonstate & (1<<i)) && (mouse_oldbuttonstate & (1<<i)) )
            Key_Event (K_MOUSE1 + i, false);
    }
    mouse_oldbuttonstate = mouse_buttonstate;
}

void IN_Move (usercmd_t *cmd)
{
    if (!mouse_avail)
        return;
   
    mouse_x *= sensitivity.value;
    mouse_y *= sensitivity.value;
   
    if ( (in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1) ))
        cmd->sidemove += m_side.value * mouse_x;
    else
        cl.viewangles[YAW] -= m_yaw.value * mouse_x;
    if (in_mlook.state & 1)
        V_StopPitchDrift ();
   
    if ( (in_mlook.state & 1) && !(in_strafe.state & 1)) {
        cl.viewangles[PITCH] += m_pitch.value * mouse_y;
        if (cl.viewangles[PITCH] > 80)
            cl.viewangles[PITCH] = 80;
        if (cl.viewangles[PITCH] < -70)
            cl.viewangles[PITCH] = -70;
    } else {
        if ((in_strafe.state & 1) && noclip_anglehack)
            cmd->upmove -= m_forward.value * mouse_y;
        else
            cmd->forwardmove -= m_forward.value * mouse_y;
    }
    mouse_x = mouse_y = 0.0;
}

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void)
{
    return 0;
}
