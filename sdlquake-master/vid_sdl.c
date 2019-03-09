// vid_sdl.h -- sdl video driver 

#include "quakedef.h"
#include "d_local.h"
#include "gfx.h"
#include "sdl_video.h"
#include "lcd_main.h"
#include "input_main.h"
#include "sdl_keysym.h"

viddef_t    vid;                // global video state
unsigned short  d_8to16table[256];

// The original defaults
//#define    BASEWIDTH    320
//#define    BASEHEIGHT   200
// Much better for high resolution displays
#define    BASEWIDTH    (320)
#define    BASEHEIGHT   (200)

#define D_SCREEN_PIX_CNT (BASEWIDTH * BASEHEIGHT)
#define D_SCREEN_BYTE_CNT (D_SCREEN_PIX_CNT * sizeof(pix_t))


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
        pal[i] = GFX_RGB(GFX_OPAQUE, r, g, b);
    }
    screen_sync(1);
    screen_set_clut(pal, 256);
    return;
}


void    VID_ShiftPalette (unsigned char *palette)
{
    VID_SetPalette(palette);
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

    screen_win_cfg(&lcd_screen);

    screen = (SDL_Surface *)Hunk_HighAllocName(BASEWIDTH * BASEHEIGHT * sizeof(pix_t) + sizeof(SDL_Surface), "screen");

    if (screen == NULL)
        Sys_Error ("Not enough memory for video mode\n");

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

#if GFX_COLOR_MODE != GFX_COLOR_MODE_CLUT
#error "Unsupported mode"
#endif

#if 1

typedef struct {
    pix_t a[4];
} scanline_t;

typedef union {
#if (GFX_COLOR_MODE == GFX_COLOR_MODE_CLUT)
    uint32_t w;
#elif (GFX_COLOR_MODE == GFX_COLOR_MODE_RGB565)
    uint64_t w;
#endif
    scanline_t sl;
} scanline_u;

#define DST_NEXT_LINE(x) (((uint32_t)(x) + BASEWIDTH * 2 * sizeof(pix_t)))
#define W_STEP (sizeof(scanline_t) / sizeof(pix_t))

void uiUpdate (vrect_t *rect, screen_t *lcd_screen)
{
    uint64_t *d_y0;
    uint64_t *d_y1;
    uint64_t pix;
    int s_y, i;
    scanline_t *scanline;
    scanline_u d_yt0, d_yt1;
    pix_t *videbuf = (pix_t *)screen->pixels;

    d_y0 = (uint64_t *)lcd_screen->buf;
    d_y1 = (uint64_t *)DST_NEXT_LINE(d_y0);

    for (s_y = 0; s_y < D_SCREEN_PIX_CNT; s_y += BASEWIDTH) {

        scanline = (scanline_t *)&videbuf[s_y];

        for (i = 0; i < BASEWIDTH; i += W_STEP) {

            d_yt0.sl = *scanline++;
            d_yt1    = d_yt0;

            d_yt0.sl.a[3] = d_yt0.sl.a[1];
            d_yt0.sl.a[2] = d_yt0.sl.a[1];
            d_yt0.sl.a[1] = d_yt0.sl.a[0];

            d_yt1.sl.a[0] = d_yt1.sl.a[2];
            d_yt1.sl.a[1] = d_yt1.sl.a[2];
            d_yt1.sl.a[2] = d_yt1.sl.a[3];

            pix = (uint64_t)(((uint64_t)d_yt1.w << 32) | d_yt0.w);
            *d_y0++     = pix;
            *d_y1++     = pix;
        }
        d_y0 = d_y1;
        d_y1 = (uint64_t *)DST_NEXT_LINE(d_y0);
    }
}

#else

void uiUpdate (vrect_t *rect, screen_t *lcd_screen)
{
    pix_t *scanline;
    pix_t *dest, *src;
    int pix_cnt;
    int x0, w, line, pix;

    dest = (pix_t *)lcd_screen->buf;
    pix_cnt = screen->w * screen->h;
    w = screen->w;

    src = (pix_t *)screen->pixels;
    line = rect->x + (rect->y * w);
    src = src + line;
    x0 = rect->x;

    for (; line < pix_cnt; line += w) {

        scanline = (pix_t *)screen->pixels;
        scanline = &scanline[line];

        for (pix = x0; pix < w; pix++) {
            dest[pix] = scanline[pix];
        }
    }
}

#endif

void    VID_Update (vrect_t *rects)
{
    vrect_t *rect;
    screen_t lcd_screen;

    screen_sync (0);
    screen_get_invis_screen(&lcd_screen);

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
        memcpy(offset, pbitmap, width);
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
    SDL_UpdateRect(screen, x, y, width, height);
}


/*
================
Sys_SendKeyEvents
================
*/
typedef i_event_t SDL_Event;

const struct usb_gamepad_to_kbd_map gamepad_to_kbd_map[K_MAX] =
{
    [K_UP]      = {K_UPARROW,       PAD_LOOK_CONTROL | PAD_LOOK_UP, 0, 0, 0},
    [K_DOWN]    = {K_DOWNARROW,     PAD_LOOK_CONTROL | PAD_LOOK_DOWN, 0, 0, 0},
    [K_LEFT]    = {K_LEFTARROW,     PAD_LOOK_CONTROL | PAD_LOOK_CENTRE, 0, 0, 0},
    [K_RIGHT]   = {K_RIGHTARROW,    PAD_LOOK_CONTROL | PAD_LOOK_CENTRE, 0, 0, 0},
    [K_K1]      = {K_TAB,           PAD_FREQ_LOW, 0, 0, 0},
    [K_K4]      = {K_SHIFT,           0, 0, 0, 0},
    [K_K3]      = {K_MOUSE1,          0, 0, 0, 0},
    [K_K2]      = {K_SPACE,        PAD_FREQ_LOW | PAD_FUNCTION, 0, 0, 0},
    [K_BL]      = {K_INS,      0, 0, 0, 0},
#if GAMEPAD_USE_FLYLOOK
    [K_BR]      = {K_DEL,    PAD_SET_FLYLOOK, 0, 0, 0},
#else
    [K_BR]      = {K_MWHEELUP,    PAD_FREQ_LOW, 0, 0, 0},
#endif
    [K_TL]      = {K_DEL,      0, 0, 0, 0},
    [K_TR]      = {K_MWHEELDOWN,    PAD_FREQ_LOW, 0, 0, 0},
    [K_START]   = {K_ENTER,         0, 0, 0, 0},
    [K_SELECT]  = {K_ESCAPE,        PAD_FREQ_LOW, 0, 0, 0},
};


void Sys_SendKeyEvents(void)
{
    SDL_Event event;
    int sym;
		qboolean state;
    int modstate;
    int8_t keys[K_MAX];
    int i, keynum;

    keynum = gamepad_read(keys);
    for (i = 0; i < keynum; i++)
    {
        state = (keys[i] > 0) ? true : false;
        if (keys[i] >= 0) {
                sym = gamepad_to_kbd_map[i].key;
                Key_Event(sym, state);
        }
#if 0
        switch (event.type) {
            case SDL_MOUSEMOTION:
                if ( (event.motion.x != (vid.width/2)) ||
                     (event.motion.y != (vid.height/2)) ) {
                    mouse_x = event.motion.xrel*10;
                    mouse_y = event.motion.yrel*10;
                    if ( (event.motion.x < ((vid.width/2)-(vid.width/4))) ||
                         (event.motion.x > ((vid.width/2)+(vid.width/4))) ||
                         (event.motion.y < ((vid.height/2)-(vid.height/4))) ||
                         (event.motion.y > ((vid.height/2)+(vid.height/4))) ) {
                        SDL_WarpMouse(vid.width/2, vid.height/2);
                    }
                }
                break;

            case SDL_QUIT:
                CL_Disconnect ();
                Host_ShutdownServer(false);        
                Sys_Quit ();
                break;
            default:
        }
#endif
    }
}

void IN_Init (void)
{
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
