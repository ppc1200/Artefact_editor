
/* Edytor plansz do gry Artefakt */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <graphics/gfxmacros.h>
#include <clib/intuition_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/graphics_protos.h>
#include <clib/exec_protos.h>

#define CursWidth   16
#define CursHeight  32

#define CursX(x)    ((x)>>4)
#define CursY(y)    ((y)>>5)
#define BaseX(x)    ((x)<<4)
#define BaseY(y)    ((y)<<5)

#define WIDTH   320
#define HEIGHT  256
#define DEPTH   5
#define MODE_ID LORES_KEY

#define BUFFER_LEN 81 /* Liczba znaków */

struct ColorSpec colspec[] =
{
    {0, 0, 0, 0},
    {1, 0, 0, 0},
    {2, 15, 15, 15},
    {3, 2, 15, 2},
    {8, 10, 15, 10},
    {4, 4, 4, 4},
    {7, 8, 15, 8},
    {12, 15, 15, 15},
    {-1}
};

struct TextAttr textattr =
{
    "topaz.font",
    8,
    FS_NORMAL,
    FPF_ROMFONT|FPF_DESIGNED
};

struct Library  *IntuitionBase;
struct Screen   *screen;
struct Window   *window;
struct RastPort *rastport; /* Podstawowy RastPort */
struct TextFont *font;
ULONG           class;
UWORD           code;
WORD            mouse_x, prev_mouse_x;
WORD            mouse_y, prev_mouse_y;
BOOL            done;
BYTE            room;
WORD            tile=1;

WORD            map[8][20] = {0};

/* Program */
main()
{
    openLibraries();
    openScreen();
    openWindow();
    loop();
    return(RETURN_OK);
}

/* Zamknij biblioteki */
void closeLibraries()
{
    CloseLibrary(IntuitionBase);
}

/* Otwórz biblioteki */
LONG openLibraries()
{
    if (IntuitionBase = OpenLibrary("intuition.library", 39))
    {
        atexit(closeLibraries);
        return(0);
    }
    exit(RETURN_FAIL);
}

/* Zamknij ekran */
void closeScreen()
{
    CloseScreen(screen);
}

/* Otwórz ekran */
LONG openScreen()
{
    if (screen = OpenScreenTags(NULL,
        SA_Left,        0,
        SA_Top,         0,
        SA_Width,       WIDTH,
        SA_Height,      HEIGHT,
        SA_Depth,       DEPTH,
        SA_DisplayID,   MODE_ID,
        SA_Quiet,       TRUE,
        SA_ShowTitle,   FALSE,
        SA_Colors,      colspec,
        SA_Font,        &textattr,
        TAG_DONE))
    {
        atexit(closeScreen);
        return(0);
    }
    exit(RETURN_FAIL);
}

/* Zamknij okno */
void closeWindow()
{
    CloseWindow(window);
}

/* Otwórz okno */
LONG openWindow()
{
    if (window = OpenWindowTags(NULL,
        WA_CustomScreen,screen,
        WA_Left,        0,
        WA_Top,         0,
        WA_Width,       screen->Width,
        WA_Height,      screen->Height,
        WA_Backdrop,    TRUE,
        WA_Borderless,  TRUE,
        WA_Activate,    TRUE,
        WA_RMBTrap,     TRUE,
        WA_ReportMouse, TRUE,
        WA_IDCMP,       IDCMP_RAWKEY|IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE,
        TAG_DONE))
    {
        atexit(closeWindow);
        rastport = window->RPort;
        font     = rastport->Font;
        return(0);
    }
    exit(RETURN_FAIL);
}

/* Pëtla gîówna */
LONG loop()
{
    struct MsgPort *mp=window->UserPort;
    struct IntuiMessage *msg;

    prev_mouse_x = mouse_x = CursX(window->MouseX);
    prev_mouse_y = mouse_y = CursY(window->MouseY);
    displayCursor(1, mouse_x, mouse_y);

    room = 'A';
    displayStatus();

    done = FALSE;
    do
    {
        WaitPort(mp);
        while (msg = GT_GetIMsg(mp))
        {
            class = msg->Class;
            code  = msg->Code;
            mouse_x = CursX(msg->MouseX);
            mouse_y = CursY(msg->MouseY);
            GT_ReplyIMsg(msg);
            switch (class)
            {
            case IDCMP_RAWKEY:
                handleRawKey();
                break;
            case IDCMP_MOUSEMOVE:
            case IDCMP_MOUSEBUTTONS:
                handleMouse();
                break;
            }
        }
    }
    while (!done);
    return(0);
}

/* Obsîuga klawiatury */
LONG handleRawKey()
{
    switch (code)
    {
    case 0x45:
        done = TRUE;
        break;
    }
}

/* Obsîuga myszy */
LONG handleMouse()
{
    static BOOL paint_mode=FALSE;
    BOOL start_paint=FALSE;

    if (class == IDCMP_MOUSEBUTTONS)
    {
        if (code == IECODE_LBUTTON)
        {
            start_paint = TRUE;
        }
        else if (code == (IECODE_LBUTTON | IECODE_UP_PREFIX))
        {
            paint_mode = FALSE;
        }
    }
    if (start_paint || mouse_x != prev_mouse_x || mouse_y != prev_mouse_y)
    {
        if (mouse_x != prev_mouse_x || mouse_y != prev_mouse_y)
        {
            displayCursor(0, prev_mouse_x, prev_mouse_y);
            displayCursor(1, mouse_x, mouse_y);
            displayStatus();
            prev_mouse_x = mouse_x;
            prev_mouse_y = mouse_y;
        }
        if (start_paint)
        {
            paint_mode = TRUE;
        }
        if (paint_mode)
        {
            if (mouse_y > 0)
            {
                map[mouse_y][mouse_x] = 4;
                displayTile(mouse_x, mouse_y);
            }
        }
    }
}

/* Wyôwietl pasek statusowy */
LONG displayStatus()
{
    BYTE bar_format[] = "Room: %c Coords: (%2d/%2d) Tile: %3d";
    BYTE bar_buffer[BUFFER_LEN];
    WORD len;

    sprintf(bar_buffer, bar_format, room, mouse_x, mouse_y, tile);
    len = strlen(bar_buffer);

    SetAPen(rastport, 2);
    SetBPen(rastport, 0);
    SetDrMd(rastport, JAM2);
    Move(rastport, 0, font->tf_Baseline);
    Text(rastport, bar_buffer, len);
}

/* Wyôwietl lub zmaű kursor */
LONG displayCursor(WORD pen, WORD x, WORD y)
{
    if (y > 0)
    {
        SetWriteMask(rastport, ~4);
        if (pen == 0)
        {
            SetAPen(rastport, 0);
        }
        else
        {
            SetAPen(rastport, 3);
            SetOPen(rastport, 8);
        }
        RectFill(rastport, BaseX(x), BaseY(y), BaseX(x) + CursWidth - 1, BaseY(y) + CursHeight - 1);
        SetWriteMask(rastport, 255);
        BNDRYOFF(rastport);
    }
}

LONG displayTile(WORD x, WORD y)
{
    if (y > 0)
    {
        SetWriteMask(rastport, 4);
        SetAPen(rastport, 4);
        RectFill(rastport, BaseX(x), BaseY(y), BaseX(x) + CursWidth - 1, BaseY(y) + CursHeight - 1);
        SetWriteMask(rastport, 255);
    }
}

/** EOF **/